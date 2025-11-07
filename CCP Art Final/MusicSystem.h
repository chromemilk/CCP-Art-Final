#include <SFML\Audio.hpp>

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




void playNextTrack() {
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

	music.setVolume( 10.f );
	music.play();
}

void playMusicTrack( const std::string &baseMusicDirectory, Levels currentLevel ) {
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