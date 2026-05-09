#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include "Settings.h"
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

enum MusicTypes
{
	JAZZ = 0,
	AMBIENT = 1
};

namespace MusicOptions
{
	std::vector<std::string> jazzTracks = {
		"St_James_Infirmary_Blues.mp3",
		"Misty_Laufey.mp3",
		"Scary_Jazz.mp3",
		"Nocture_Interlude_Laufey.mp3",
		"Midnight_stars_and_you.mp3",
		"A_Night_To_Remember_Laufey.mp3",
		"I_Wish_You_Love_Laufey.mp3",
	};

	std::vector<std::string> caveSounds = {
		"Creepy_Cave_Noise.mp3"
	};
}

static sf::Music music;
static int jazzIndex = 0;
static int caveIndex = 0;
static MusicTypes g_currentMusicType;
static std::string g_baseMusicDirectory;
static bool g_musicInitialized = false;
static float g_musicVolume = 0.f;
static bool g_autoVolumeCalibrated = false;
static sf::Vector3f g_musicSourcePosition{ 10.0f, 9.0f, 0.0f };
static float g_musicMinDistance = 3.25f;
static float g_musicAttenuation = 0.55f;
static float g_musicOcclusionMix = 0.0f;
static float g_musicDistanceMix = 1.0f;
static float g_musicPerceivedDistance = -1.0f;

static std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sfxBuffers;
static std::vector<std::unique_ptr<sf::Sound>> sfxPool;
static int sfxPoolIndex = 0;

static void playSFX(const std::string& path) {
	if (sfxBuffers.find(path) == sfxBuffers.end()) {
		auto buffer = std::make_unique<sf::SoundBuffer>();
		if (buffer->loadFromFile(path)) {
			sfxBuffers[path] = std::move(buffer);
		}
		else {
			return;
		}
	}

	if (sfxPool.size() < 32) {
		sfxPool.push_back(std::make_unique<sf::Sound>(*sfxBuffers[path]));
		sfxPool.back()->play();
		sfxPoolIndex = (int)(sfxPool.size() % 32);
	}
	else {
		sfxPool[sfxPoolIndex]->setBuffer(*sfxBuffers[path]);
		sfxPool[sfxPoolIndex]->play();
		sfxPoolIndex = (sfxPoolIndex + 1) % 32;
	}
}

static float computeDistanceGainFromPerceivedDistance(float perceivedDistance) {
	const float ref = std::max(1.0f, g_musicMinDistance * 3.6f);
	const float x = perceivedDistance / ref;
	const float spreading = 1.0f / std::sqrt(1.0f + 0.22f * x * x);
	const float airAbsorption = std::exp(-0.010f * perceivedDistance);
	return std::clamp(spreading * airAbsorption, 0.30f, 1.0f);
}

static void applyMusicMix() {
	const float occlusionVolumeMul = 1.0f - 0.45f * std::clamp(g_musicOcclusionMix, 0.0f, 1.0f);
	const float distanceVolumeMul = std::clamp(g_musicDistanceMix, 0.0f, 1.0f);

	music.setVolume(std::clamp(g_musicVolume * occlusionVolumeMul * distanceVolumeMul, 0.0f, 100.0f));
	music.setPitch(1.0f);
}

void setMusicSourcePosition(float x, float y, float z = 0.0f) {
	g_musicSourcePosition = sf::Vector3f(x, y, z);
	g_musicPerceivedDistance = -1.0f;
}

sf::Vector3f getMusicSourcePosition() {
	return g_musicSourcePosition;
}

void updateMusicListener(float x, float y, float dirX, float dirY) {
	static Uint32 lastTick = SDL_GetTicks();
	const Uint32 nowTick = SDL_GetTicks();
	float dt = (nowTick - lastTick) * (1.0f / 1000.0f);
	lastTick = nowTick;
	dt = std::clamp(dt, 0.0f, 0.05f);

	const float dx = g_musicSourcePosition.x - x;
	const float dy = g_musicSourcePosition.y - y;
	const float actualDistance = std::sqrt(dx * dx + dy * dy);

	if (g_musicPerceivedDistance < 0.0f)
	{
		g_musicPerceivedDistance = actualDistance;
	}

	constexpr float kAcousticSpeedUnitsPerSecond = 34.0f;
	const float maxStep = kAcousticSpeedUnitsPerSecond * dt;
	const float delta = std::clamp(actualDistance - g_musicPerceivedDistance, -maxStep, maxStep);
	g_musicPerceivedDistance += delta;

	const float targetDistanceMix = computeDistanceGainFromPerceivedDistance(g_musicPerceivedDistance);
	const float alpha = std::clamp(dt * 4.2f, 0.0f, 1.0f);
	g_musicDistanceMix += (targetDistanceMix - g_musicDistanceMix) * alpha;

	applyMusicMix();
}

void updateMusicOcclusion(bool blocked, float dt) {
	const float target = blocked ? 1.0f : 0.0f;
	const float speed = blocked ? 6.0f : 3.4f;
	const float alpha = std::clamp(dt * speed, 0.0f, 1.0f);

	g_musicOcclusionMix += (target - g_musicOcclusionMix) * alpha;
	if (std::fabs(g_musicOcclusionMix - target) < 0.001f)
	{
		g_musicOcclusionMix = target;
	}

	applyMusicMix();
}

void configureMusicSpatialForLevel(Levels currentLevel) {
	if (currentLevel == Levels::MUSEUM || currentLevel == Levels::MUSEUM_UPPER)
	{
		g_musicMinDistance = 8.5f;
		g_musicAttenuation = 0.03f;
		setMusicSourcePosition(10.0f, 9.0f, 0.0f);
	}
	else if (currentLevel == Levels::CAVE)
	{
		g_musicMinDistance = 6.5f;
		g_musicAttenuation = 0.05f;
		setMusicSourcePosition(5.5f, 5.5f, 0.0f);
	}
}

void setMusicVolume(float volume) {
	g_musicVolume = std::clamp(volume, 0.f, 100.f);
	applyMusicMix();
}

float getMusicVolume() {
	return g_musicVolume;
}

bool calibrateMusicVolumeFromMic(int captureMs = 1200, bool userReset = false) {
	if (g_autoVolumeCalibrated && userReset == false) return true;

	if (!config::autoMusicVolume)
	{
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return true;
	}

	if (!sf::SoundBufferRecorder::isAvailable())
	{
		std::cout << "Mic recorder not available, using fallback volume." << std::endl;
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return false;
	}

	sf::SoundBufferRecorder recorder;
	if (!recorder.start(22050))
	{
		std::cout << "Mic recorder failed to start, using fallback volume." << std::endl;
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return false;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(captureMs));
	recorder.stop();

	const sf::SoundBuffer& buf = recorder.getBuffer();
	const auto* samples = buf.getSamples();
	size_t count = buf.getSampleCount();
	if (!samples || count == 0)
	{
		std::cout << "No mic samples captured, using fallback volume." << std::endl;
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return false;
	}

	double sumSq = 0.0;
	for (size_t i = 0; i < count; ++i)
	{
		double s = double(samples[i]) / 32768.0;
		sumSq += s * s;
	}
	double rms = std::sqrt(sumSq / double(count));
	float ambient = float(std::clamp(rms, 0.0, 0.18));
	float norm = ambient / 0.18f;
	float target = 8.0f + norm * 44.0f;

	setMusicVolume(target);
	config::musicVolume = target;
	std::cout << "Auto music volume calibrated to " << int(target) << "% (ambient=" << ambient << ")" << std::endl;
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
		if (MusicOptions::jazzTracks.empty()) return;
		selectedFile = MusicOptions::jazzTracks[jazzIndex];
		jazzIndex = (jazzIndex + 1) % MusicOptions::jazzTracks.size();
	}
	else
	{
		if (MusicOptions::caveSounds.empty()) return;
		selectedFile = MusicOptions::caveSounds[caveIndex];
		caveIndex = (caveIndex + 1) % MusicOptions::caveSounds.size();
	}

	std::string finalPath = g_baseMusicDirectory + "\\" + selectedFile;
	std::cout << "Playing: " + finalPath << std::endl;

	music.stop();

	if (!music.openFromFile(finalPath))
	{
		std::cout << "Failed to get music: " << finalPath << std::endl;
		return;
	}

	music.setSpatializationEnabled(true);
	applyMusicMix();
	music.play();
}

void playMusicTrack(const std::string& baseMusicDirectory, Levels currentLevel) {
	if (config::useMusic == false)
	{
		music.stop();
		return;
	}

	g_baseMusicDirectory = baseMusicDirectory;
	configureMusicSpatialForLevel(currentLevel);

	if (currentLevel != Levels::MUSEUM && currentLevel != Levels::MUSEUM_UPPER && currentLevel != Levels::CAVE) return;

	MusicTypes selectedType = (currentLevel == Levels::CAVE) ? MusicTypes::AMBIENT : MusicTypes::JAZZ;

	if (selectedType != g_currentMusicType || music.getStatus() == sf::SoundStream::Status::Stopped)
	{
		g_currentMusicType = selectedType;
		g_musicInitialized = true;

		playNextTrack();
	}
}

void playPickup(const std::string& baseMusicDirectory) {
	std::string soundPath = baseMusicDirectory + "\\pickup.wav";
	playSFX(soundPath);
}

void playFootstep(const std::string& baseMusicDirectory) {
	std::string soundPath = baseMusicDirectory + "\\Sound 01.wav";
	playSFX(soundPath);
}

void playFailedDoorOpen(const std::string& baseMusicDirectory) {
	std::string soundPath = baseMusicDirectory + "\\failed_door_open.wav";
	playSFX(soundPath);
}

void playDoorCreak(const std::string& baseMusicDirectory) {
	std::string soundPath = baseMusicDirectory + "\\door_creak.wav";
	playSFX(soundPath);
}

void playPaperRustle(const std::string& baseMusicDirectory) {
	std::vector<std::string> candidates = {
		"Paper_Rustle_01.wav",
		"Paper_Rustle_02.wav",
		"paper_rustle.wav"
	};

	int start = int(GetTickCount64() % candidates.size());
	for (int i = 0; i < (int)candidates.size(); ++i)
	{
		int idx = (start + i) % candidates.size();
		std::string soundPath = baseMusicDirectory + "\\" + candidates[idx];
		playSFX(soundPath);
		break;
	}
}

static sf::SoundBuffer g_heartbeatBuffer;
static bool g_heartbeatBufferReady = false;
static std::shared_ptr<sf::Sound> g_heartbeatSound;
static std::string g_heartbeatBaseDirectory;

static void stopHeartbeat() {
	if (g_heartbeatSound)
	{
		g_heartbeatSound->stop();
		g_heartbeatSound.reset();
	}
}

static void playHeartbeat(const std::string& baseMusicDirectory) {
	const std::string soundPath = baseMusicDirectory + "\\heartbeat.wav";

	if (!g_heartbeatBufferReady || g_heartbeatBaseDirectory != baseMusicDirectory)
	{
		g_heartbeatBaseDirectory = baseMusicDirectory;
		g_heartbeatBufferReady = g_heartbeatBuffer.loadFromFile(soundPath);
	}

	if (!g_heartbeatBufferReady)
	{
		return;
	}

	stopHeartbeat();

	g_heartbeatSound = std::make_shared<sf::Sound>(g_heartbeatBuffer);
	g_heartbeatSound->setLooping(true);
	g_heartbeatSound->setVolume(100.0f);
	g_heartbeatSound->play();
}