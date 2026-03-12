#include <SFML\Audio.hpp>
#include <SFML\System.hpp>
#include "Settings.h"
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

enum MusicTypes
{
	JAZZ = 0,
	AMBIENT = 1
};

namespace MusicOptions
{
	std::vector<std::string> jazzTracks = {
		"Nocture_Interlude_Laufey.mp3",
		"A_Night_To_Remember_Laufey.mp3",
		"Misty_Laufey.mp3",
		"I_Wish_You_Love_Laufey.mp3",
	};

	std::vector<std::string> caveSounds = {
	"Creepy_Cave_Noise.mp3"
	//"Initial_Noise.mp3",
	};

}

static sf::Music music;
static int jazzIndex = 0;
static int caveIndex = 0;
static MusicTypes g_currentMusicType;     // Tracks the current playlist
static std::string g_baseMusicDirectory;  // Stores the path for the update function
static bool g_musicInitialized = false;   // Prevents update loop from running early
static float g_musicVolume = 0.f;
static bool g_autoVolumeCalibrated = false;


void setMusicVolume( float volume ) {
	g_musicVolume = std::clamp( volume, 0.f, 100.f );
	music.setVolume( g_musicVolume );
}

float getMusicVolume() {
	return g_musicVolume;
}

bool calibrateMusicVolumeFromMic(int captureMs = 1200, bool userReset = false ) {
	if (g_autoVolumeCalibrated && userReset == false) return true;

	if (!config::autoMusicVolume)
	{
		setMusicVolume( config::musicVolume );
		g_autoVolumeCalibrated = true;
		return true;
	}

	if (!sf::SoundBufferRecorder::isAvailable())
	{
		std::cout << "Mic recorder not available, using fallback volume." << std::endl;
		setMusicVolume( config::musicVolume );
		g_autoVolumeCalibrated = true;
		return false;
	}

	sf::SoundBufferRecorder recorder;
	if (!recorder.start( 22050 ))
	{
		std::cout << "Mic recorder failed to start, using fallback volume." << std::endl;
		setMusicVolume( config::musicVolume );
		g_autoVolumeCalibrated = true;
		return false;
	}

	std::this_thread::sleep_for( std::chrono::milliseconds( captureMs ) );
	recorder.stop();

	const sf::SoundBuffer &buf = recorder.getBuffer();
	const auto *samples = buf.getSamples();
	size_t count = buf.getSampleCount();
	if (!samples || count == 0)
	{
		std::cout << "No mic samples captured, using fallback volume." << std::endl;
		setMusicVolume( config::musicVolume );
		g_autoVolumeCalibrated = true;
		return false;
	}

	double sumSq = 0.0;
	for (size_t i = 0; i < count; ++i)
	{
		double s = double( samples[ i ] ) / 32768.0;
		sumSq += s * s;
	}
	double rms = std::sqrt( sumSq / double( count ) );
	float ambient = float( std::clamp( rms, 0.0, 0.18 ) );
	float norm = ambient / 0.18f;
	float target = 26.0f + norm * 44.0f; // 26..70

	setMusicVolume( target );
	config::musicVolume = target;
	std::cout << "Auto music volume calibrated to " << int( target ) << "% (ambient=" << ambient << ")" << std::endl;
	config::calibratedVolume = target;

	g_autoVolumeCalibrated = true;
	return true;
}

void playNextTrack() {
	if (config::useMusic == false)
	{
		music.stop();
		return;
	}

	if (g_baseMusicDirectory.empty())
	{
		return; 
	}

	std::string selectedFile;
	if (g_currentMusicType == MusicTypes::JAZZ)
	{
		if (MusicOptions::jazzTracks.empty()) return; // No music to play
		selectedFile = MusicOptions::jazzTracks[ jazzIndex ];
		jazzIndex = (jazzIndex + 1) % MusicOptions::jazzTracks.size();
	}
	else // AMBIENT
	{
		if (MusicOptions::caveSounds.empty()) return; // No music to play
		selectedFile = MusicOptions::caveSounds[ caveIndex ];
		caveIndex = (caveIndex + 1) % MusicOptions::caveSounds.size();
	}

	std::string finalPath = g_baseMusicDirectory + "\\" + selectedFile;
	std::cout << "Playing: " + finalPath << std::endl;

	music.stop(); 

	if (!music.openFromFile( finalPath ))
	{
		std::cout << "Failed to get music: " << finalPath << std::endl;
		return;
	}

	music.setVolume( g_musicVolume );
	music.play();
}

void playMusicTrack( const std::string &baseMusicDirectory, Levels currentLevel ) {

	if (config::useMusic == false)
	{
		music.stop();
		return;
	}

	g_baseMusicDirectory = baseMusicDirectory;

	if (currentLevel != Levels::MUSEUM && currentLevel != Levels::CAVE) return;

	MusicTypes selectedType = (currentLevel == Levels::MUSEUM) ? MusicTypes::JAZZ : MusicTypes::AMBIENT;

	if (selectedType != g_currentMusicType || music.getStatus() == sf::SoundStream::Status::Stopped)
	{
		g_currentMusicType = selectedType; // Set the new type
		g_musicInitialized = true;       // Allow the update loop to work

		playNextTrack();
	}
}

void playPickup(const std::string& baseMusicDirectory) {

	std::string soundPath = baseMusicDirectory + "\\pickup.wav";

	PlaySoundA(soundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME);
}
void playFootstep( const std::string &baseMusicDirectory ) {

    std::string soundPath = baseMusicDirectory + "\\Sound 01.wav";

	PlaySoundA( soundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NOSTOP );
}

void playFailedDoorOpen(const std::string& baseMusicDirectory) {
	std::string soundPath = baseMusicDirectory + "\\" + "failed_door_open.wav";

	if (PlaySoundA(soundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME)) return;
}

void playDoorCreak( const std::string &baseMusicDirectory ) {
	std::string soundPath = baseMusicDirectory + "\\" + "door_creak.wav";

	if (PlaySoundA(soundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME)) return;

}

void playPaperRustle( const std::string &baseMusicDirectory ) {
	std::vector<std::string> candidates = {
		"Paper_Rustle_01.wav",
		"Paper_Rustle_02.wav",
		"paper_rustle.wav"
	};

	int start = int( GetTickCount64() % candidates.size() );
	for (int i = 0; i < (int)candidates.size(); ++i)
	{
		int idx = (start + i) % candidates.size();
		std::string soundPath = baseMusicDirectory + "\\" + candidates[ idx ];
		if (PlaySoundA( soundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME )) return;
	}
}