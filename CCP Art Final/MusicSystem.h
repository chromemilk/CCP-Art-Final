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

void playMusicTrack( const std::string &baseMusicDirectory, Levels currentLevel) {
	// Track path will just be the relative path then we will append to our base music directory
	MusicTypes selectedType = currentLevel == Levels::MUSEUM ? MusicTypes::JAZZ : MusicTypes::AMBIENT;

	static sf::Music music;
	static int jazzIndex = 0;
	static int caveIndex = 0;

	std::string selectedFile;

	if (selectedType == MusicTypes::JAZZ)
	{
		if (MusicOptions::jazzTracks.empty())
		{
			return;
		}

		selectedFile = MusicOptions::jazzTracks[ jazzIndex ];
		// Cycle
		jazzIndex = (jazzIndex + 1) % MusicOptions::jazzTracks.size();
	}
	else if (selectedType == MusicTypes::AMBIENT)
	{
		if (MusicOptions::caveSounds.empty())
		{
			return;
		}

		selectedFile = MusicOptions::caveSounds[ caveIndex ];
		// Cycle
		caveIndex = (caveIndex + 1) % MusicOptions::caveSounds.size();
	}

	std::string finalPath = baseMusicDirectory + "\\" + selectedFile;
	std::cout << "playing: " + finalPath;

	music.stop();

	if (!music.openFromFile( finalPath ))
	{
		std::cout << "Failed to get music" << std::endl;

		return;
	}

	music.setVolume(10.f);

	music.play();
}