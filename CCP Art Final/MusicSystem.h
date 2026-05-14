#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include "Settings.h"

#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#pragma comment(lib, "ole32.lib")

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <numeric>

enum MusicTypes { JAZZ = 0, AMBIENT = 1 };

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

static sf::Music  music;
static int jazzIndex = 0;
static int caveIndex = 0;
static MusicTypes g_currentMusicType;
static std::string g_baseMusicDirectory;
static bool g_musicInitialized = false;
static float g_musicVolume = 0.f;   // desired perceptual 0-100
static bool g_autoVolumeCalibrated = false;

static sf::Vector3f g_musicSourcePosition{ 10.0f, 9.0f, 0.0f };
static float g_musicMinDistance = 3.25f;
static float g_musicAttenuation = 0.55f;
static float g_musicOcclusionMix = 0.0f;   // 0=open  1=fully blocked
static float g_musicDistanceMix = 1.0f;   // 0=silent 1=full volume
static float g_musicPerceivedDistance = -1.0f;  // <0 = uninitialised

static float g_systemVolumeScale = 1.0f;


static float g_calibratedAmbientRMS = 0.0f;

// SFX pool
static std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sfxBuffers;
static std::vector<std::unique_ptr<sf::Sound>> sfxPool;
static int sfxPoolIndex = 0;



static float querySystemMasterVolume()
{
	float vol = 1.0f;

	IMMDeviceEnumerator* pEnum = nullptr;
	IMMDevice* pDev = nullptr;
	IAudioEndpointVolume* pEPVol = nullptr;

	// CoInitialize is safe to call repeatedly; use Ex to avoid clobbering the
	// apartment type of whatever thread the game already set up.
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	auto cleanup = [&]()
		{
			if (pEPVol) pEPVol->Release();
			if (pDev)   pDev->Release();
			if (pEnum)  pEnum->Release();
		};

	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum)))
	{
		cleanup(); return vol;
	}
	if (FAILED(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev)))
	{
		cleanup(); return vol;
	}
	if (FAILED(pDev->Activate(__uuidof(IAudioEndpointVolume),
		CLSCTX_ALL, nullptr, (void**)&pEPVol)))
	{
		cleanup(); return vol;
	}

	BOOL muted = FALSE;
	pEPVol->GetMute(&muted);
	if (!muted)
		pEPVol->GetMasterVolumeLevelScalar(&vol);
	else
		vol = 0.01f; // muted but not zero so we don't divide by zero later

	cleanup();
	return std::clamp(vol, 0.01f, 1.0f);
}


static float computeSFXVolume(float baseVolume = 100.0f)
{
	// Boost SFX proportionally to ambient noise so they stay perceptible.
	// g_calibratedAmbientRMS is in [0, 0.18]; norm is [0, 1].
	float norm = std::clamp(g_calibratedAmbientRMS / 0.18f, 0.0f, 1.0f);

	float boost = 1.0f + 0.5f * norm;

	// Compensate for system volume so "100%" always sounds like 100%.
	float systemComp = std::clamp(1.0f / g_systemVolumeScale, 1.0f, 4.0f);

	return std::clamp(baseVolume * boost * systemComp, 0.0f, 100.0f);
}

static void playSFX(const std::string& path, float volume = 100.0f)
{
	if (sfxBuffers.find(path) == sfxBuffers.end())
	{
		auto buffer = std::make_unique<sf::SoundBuffer>();
		if (buffer->loadFromFile(path))
			sfxBuffers[path] = std::move(buffer);
		else
			return;
	}

	const float adjustedVol = computeSFXVolume(volume);

	if (sfxPool.size() < 32)
	{
		sfxPool.push_back(std::make_unique<sf::Sound>(*sfxBuffers[path]));
		sfxPool.back()->setVolume(adjustedVol);
		sfxPool.back()->play();
		sfxPoolIndex = (int)(sfxPool.size() % 32);
	}
	else
	{
		sfxPool[sfxPoolIndex]->setBuffer(*sfxBuffers[path]);
		sfxPool[sfxPoolIndex]->setVolume(adjustedVol);
		sfxPool[sfxPoolIndex]->play();
		sfxPoolIndex = (sfxPoolIndex + 1) % 32;
	}
}

static float computeDistanceGain(float perceivedDistance)
{
	if (perceivedDistance <= g_musicMinDistance)
		return 1.0f;

	// Normalised distance beyond the min-distance radius.
	const float r = perceivedDistance / std::max(g_musicMinDistance, 0.1f);

	const float invSq = 1.0f / (r * r);
	const float rolloff = std::lerp(1.0f, invSq, std::clamp(g_musicAttenuation * 10.0f, 0.0f, 1.0f));

	// Air absorption — models how sound energy is lost over distance.
	const float absorption = std::exp(-0.008f * std::max(0.0f, perceivedDistance - g_musicMinDistance));

	return std::clamp(rolloff * absorption, 0.08f, 1.0f);
}

static void applyMusicMix()
{
	const float occlusionMul = 1.0f - 0.55f * std::clamp(g_musicOcclusionMix, 0.0f, 1.0f);
	const float distanceMul = std::clamp(g_musicDistanceMix, 0.0f, 1.0f);

	// Compensate for system volume: if Windows is at 50%, we double internal
	// volume so the perceived output matches g_musicVolume.
	const float sysComp = std::clamp(1.0f / g_systemVolumeScale, 1.0f, 4.0f);
	const float finalVol = std::clamp(g_musicVolume * occlusionMul * distanceMul * sysComp,
		0.0f, 100.0f);

	music.setVolume(finalVol);
}

void setMusicSourcePosition(float x, float y, float z = 0.0f)
{
	g_musicSourcePosition = sf::Vector3f(x, y, z);
	g_musicPerceivedDistance = -1.0f; // force re-initialisation next update
}

sf::Vector3f getMusicSourcePosition()
{
	return g_musicSourcePosition;
}

// Call this every frame with the player's tile-space position and facing.
// Uses framerate-independent exponential smoothing for both perceived
// distance and the distance-mix volume so that frame-rate spikes no longer
// snap the audio to extremes.
void updateMusicListener(float x, float y, float /*dirX*/, float /*dirY*/)
{
	static Uint32 lastTick = SDL_GetTicks();
	const Uint32 nowTick = SDL_GetTicks();
	float dt = std::clamp((nowTick - lastTick) * 0.001f, 0.0f, 0.05f);
	lastTick = nowTick;

	// Euclidean distance in tile-space (each tile = 1 unit).
	const float dx = g_musicSourcePosition.x - x;
	const float dy = g_musicSourcePosition.y - y;
	const float actualDistance = std::sqrt(dx * dx + dy * dy);

	// Initialise perceived distance on first call or after source repositions.
	if (g_musicPerceivedDistance < 0.0f)
		g_musicPerceivedDistance = actualDistance;

	constexpr float kAcousticSpeedUnitsPerSec = 34.0f;
	const float maxStep = kAcousticSpeedUnitsPerSec * dt;
	const float delta = std::clamp(actualDistance - g_musicPerceivedDistance, -maxStep, maxStep);
	g_musicPerceivedDistance += delta;

	constexpr float kDistanceTau = 0.18f; // ~180 ms smoothing
	const float alpha = 1.0f - std::exp(-dt / kDistanceTau);

	const float targetDistanceMix = computeDistanceGain(g_musicPerceivedDistance);
	g_musicDistanceMix += (targetDistanceMix - g_musicDistanceMix) * alpha;

	applyMusicMix();
}

void updateMusicOcclusion(bool blocked, float dt)
{
	// Faster to occlude than to un-occlude (physically: blocking is abrupt,
	// sound "leaks back" around obstacles gradually).
	constexpr float kOccludeTau = 0.10f; // ~100 ms to go muffled
	constexpr float kUnoccludeTau = 0.28f; // ~280 ms to brighten back up

	const float tau = blocked ? kOccludeTau : kUnoccludeTau;
	const float alpha = 1.0f - std::exp(-dt / tau);
	const float target = blocked ? 1.0f : 0.0f;

	g_musicOcclusionMix += (target - g_musicOcclusionMix) * alpha;
	if (std::fabs(g_musicOcclusionMix - target) < 0.001f)
		g_musicOcclusionMix = target;

	applyMusicMix();
}


void configureMusicSpatialForLevel(Levels currentLevel)
{
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


void setMusicVolume(float volume)
{
	g_musicVolume = std::clamp(volume, 0.f, 100.f);
	applyMusicMix();
}

float getMusicVolume() { return g_musicVolume; }

bool calibrateMusicVolumeFromMic(int captureMs = 5000)
{
	if (g_autoVolumeCalibrated) return true;

	// Always query system volume — needed even if we're not using mic calibration.
	g_systemVolumeScale = querySystemMasterVolume();
	std::cout << "System master volume: " << int(g_systemVolumeScale * 100.f) << "%" << std::endl;

	if (!config::autoMusicVolume)
	{
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return true;
	}

	if (!sf::SoundBufferRecorder::isAvailable())
	{
		std::cout << "Mic not available, using fallback volume." << std::endl;
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return false;
	}

	constexpr int kWindows = 5;
	const int windowMs = std::max(captureMs / kWindows, 200);
	std::vector<float> windowRMS;
	windowRMS.reserve(kWindows);

	for (int w = 0; w < kWindows; ++w)
	{
		sf::SoundBufferRecorder recorder;
		if (!recorder.start(22050))
		{
			std::cout << "Mic failed to start (window " << w << "), skipping." << std::endl;
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(windowMs));
		recorder.stop();

		const sf::SoundBuffer& buf = recorder.getBuffer();
		const int16_t* samples = buf.getSamples();
		const std::size_t      count = buf.getSampleCount();
		if (!samples || count == 0) continue;

		double sumSq = 0.0;
		for (std::size_t i = 0; i < count; ++i)
		{
			double s = static_cast<double>(samples[i]) / 32768.0;
			sumSq += s * s;
		}
		windowRMS.push_back(static_cast<float>(std::sqrt(sumSq / static_cast<double>(count))));
	}

	if (windowRMS.empty())
	{
		std::cout << "No valid mic windows captured, using fallback volume." << std::endl;
		setMusicVolume(config::musicVolume);
		g_autoVolumeCalibrated = true;
		return false;
	}

	std::sort(windowRMS.begin(), windowRMS.end());

	const float noiseFloor = windowRMS.front();

	const float medianRMS = windowRMS[windowRMS.size() / 2];

	const float effectiveAmbient = std::max(0.0f, medianRMS - noiseFloor * 0.5f);

	// Save normalised ambient level for SFX scaling (0 = silent, 1 = very loud).
	g_calibratedAmbientRMS = std::clamp(effectiveAmbient / 0.18f, 0.0f, 1.0f);
	const float norm = g_calibratedAmbientRMS;
	float target = 18.0f + norm * norm * 47.0f; // quadratic — steeper for loud rooms

	const float sysComp = std::clamp(1.0f / g_systemVolumeScale, 1.0f, 4.0f);
	target = std::clamp(target * sysComp, 0.0f, 100.0f);

	setMusicVolume(target);
	config::musicVolume = target;
	config::calibratedVolume = target;

	std::cout << "Calibration: noiseFloor=" << noiseFloor
		<< "  medianRMS=" << medianRMS
		<< "  effectiveAmbient=" << effectiveAmbient
		<< "  systemVolume=" << int(g_systemVolumeScale * 100.f) << "%"
		<< "  -> musicVolume=" << int(target) << "%" << std::endl;

	g_autoVolumeCalibrated = true;
	return true;
}


void playNextTrack()
{
	if (!config::useMusic) { music.stop(); return; }
	if (g_baseMusicDirectory.empty()) return;

	std::string selectedFile;
	if (g_currentMusicType == MusicTypes::JAZZ)
	{
		if (MusicOptions::jazzTracks.empty()) return;
		selectedFile = MusicOptions::jazzTracks[jazzIndex];
		jazzIndex = (jazzIndex + 1) % (int)MusicOptions::jazzTracks.size();
	}
	else
	{
		if (MusicOptions::caveSounds.empty()) return;
		selectedFile = MusicOptions::caveSounds[caveIndex];
		caveIndex = (caveIndex + 1) % (int)MusicOptions::caveSounds.size();
	}

	const std::string finalPath = g_baseMusicDirectory + "\\" + selectedFile;
	std::cout << "Playing: " << finalPath << std::endl;

	music.stop();
	if (!music.openFromFile(finalPath))
	{
		std::cout << "Failed to load: " << finalPath << std::endl;
		return;
	}

	music.setSpatializationEnabled(false);
	music.setLooping(false); 
	applyMusicMix();
	music.play();
}


void tickMusicSystem()
{
	if (!g_musicInitialized) return;
	if (!config::useMusic)   return;
	if (music.getStatus() == sf::SoundStream::Status::Stopped)
		playNextTrack();
}

void playMusicTrack(const std::string& baseMusicDirectory, Levels currentLevel)
{
	if (!config::useMusic) { music.stop(); return; }

	g_baseMusicDirectory = baseMusicDirectory;
	configureMusicSpatialForLevel(currentLevel);

	if (currentLevel != Levels::MUSEUM
		&& currentLevel != Levels::MUSEUM_UPPER
		&& currentLevel != Levels::CAVE)
		return;

	const MusicTypes selectedType = (currentLevel == Levels::CAVE)
		? MusicTypes::AMBIENT
		: MusicTypes::JAZZ;

	if (selectedType != g_currentMusicType
		|| music.getStatus() == sf::SoundStream::Status::Stopped)
	{
		g_currentMusicType = selectedType;
		g_musicInitialized = true;
		playNextTrack();
	}
}


void playPickup(const std::string& baseMusicDirectory)
{
	playSFX(baseMusicDirectory + "\\pickup.wav");
}

void playFootstep(const std::string& baseMusicDirectory)
{

	playSFX(baseMusicDirectory + "\\Sound 01.wav", 90.0f);
}

void playFailedDoorOpen(const std::string& baseMusicDirectory)
{
	playSFX(baseMusicDirectory + "\\failed_door_open.wav");
}

void playDoorCreak(const std::string& baseMusicDirectory)
{
	playSFX(baseMusicDirectory + "\\door_creak.wav");
}

void playPaperRustle(const std::string& baseMusicDirectory)
{
	static const std::vector<std::string> candidates = {
		"Paper_Rustle_01.wav",
		"Paper_Rustle_02.wav",
		"paper_rustle.wav"
	};
	const int start = static_cast<int>(GetTickCount64() % candidates.size());
	const int idx = start % static_cast<int>(candidates.size());
	playSFX(baseMusicDirectory + "\\" + candidates[idx]);
}


static sf::SoundBuffer g_heartbeatBuffer;
static bool g_heartbeatBufferReady = false;
static std::shared_ptr<sf::Sound> g_heartbeatSound;
static std::string g_heartbeatBaseDirectory;

static void stopHeartbeat()
{
	if (g_heartbeatSound)
	{
		g_heartbeatSound->stop();
		g_heartbeatSound.reset();
	}
}


static void playHeartbeat(const std::string& baseMusicDirectory)
{
	const std::string soundPath = baseMusicDirectory + "\\heartbeat.wav";

	if (!g_heartbeatBufferReady || g_heartbeatBaseDirectory != baseMusicDirectory)
	{
		g_heartbeatBaseDirectory = baseMusicDirectory;
		g_heartbeatBufferReady = g_heartbeatBuffer.loadFromFile(soundPath);
	}
	if (!g_heartbeatBufferReady) return;

	stopHeartbeat();

	g_heartbeatSound = std::make_shared<sf::Sound>(g_heartbeatBuffer);
	g_heartbeatSound->setLooping(true);

	const float sysComp = std::clamp(1.0f / g_systemVolumeScale, 1.0f, 4.0f);
	const float vol = std::clamp(100.0f * sysComp * 1.25f, 0.0f, 100.0f);
	g_heartbeatSound->setVolume(vol);
	g_heartbeatSound->play();
}