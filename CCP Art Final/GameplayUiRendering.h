#pragma once

#include "GameEngine.h"
#include "GameDataTypes.h"
#include "TextBitmap.h"

static std::string normalizeMindTrapTerminalText( std::string s );
static bool isMindTrapDenialOption( const std::string &optionText );
static std::string mindTrapCorruptOptionText( std::string text, float intensity );


struct MindTrapMovieFrame {
    std::vector<std::string> lines;
    std::string subtitle;
    float timestamp; // At what second (from 0 to 15) does this frame APPEAR?
};

static std::vector<std::vector<std::vector<MindTrapMovieFrame>>> g_mindTrapMovies;

static void initMindTrapMovies() {
    if (!g_mindTrapMovies.empty()) return;

    g_mindTrapMovies.resize(5); // 5 Phases
    for (int i = 0; i < 5; ++i) g_mindTrapMovies[i].resize(3); // 3 Choices per Phase


    // 
    // Choice 0: "I CAN SEE MY OWN BREATH"
    // Theme: Person exhaling -> Cloud of smoke -> Cluster of crystals -> Ice and icicles -> Freezing
    // 
    g_mindTrapMovies[0][0] = {
        // "YOU EXHALE." (Person exhaling)
        { { "   .----.   ", "  / o  o \\  ", " |  /\\  | ", " | .||. | ", "  \\ ---- /  " }, "YOU EXHALE.", 0.00f },
        { { "   .----.   ", "  / o  o \\  ", " |  /\\  | ", " | .||. |~", "  \\ ---- /  " }, "YOU EXHALE.", 0.60f },
        { { "   .----.   ", "  / o  o \\  ", " |  /\\  |~", " | .||. |~", "  \\ ---- /~ " }, "YOU EXHALE.", 1.20f },
        { { "   .----.   ", "  / -  - \\ ~", " |  /\\  |~", " | .||. |~~", "  \\ ---- /~~" }, "YOU EXHALE.", 1.80f },
        { { "   .----.  ~", "  / -  - \\~~", " |  /\\  |~~", " | .||. |~~", "  \\ ---- /~~" }, "YOU EXHALE.", 2.40f },
        // "THE VAPOR HANGS IN THE AIR." (Cloud of smoke)
        { { "    ~~~~    ", "  ~~    ~~  ", " ~~      ~~ ", "  ~~    ~~  ", "    ~~~~    " }, "THE VAPOR HANGS IN THE AIR.", 3.00f },
        { { "   ~~~~~~   ", " ~~~~  ~~~~ ", "~~~~    ~~~~", " ~~~~  ~~~~ ", "   ~~~~~~   " }, "THE VAPOR HANGS IN THE AIR.", 3.60f },
        { { "  ~~~~~~~~  ", " ~~~~~~~~~~ ", "~~~~~~~~~~~~", " ~~~~~~~~~~ ", "  ~~~~~~~~  " }, "THE VAPOR HANGS IN THE AIR.", 4.20f },
        { { " ~~~~~~~~~~ ", "~~~~~~~~~~~~", "~~~~~~~~~~~~", "~~~~~~~~~~~~", " ~~~~~~~~~~ " }, "THE VAPOR HANGS IN THE AIR.", 4.80f },
        { { "~~~~~~~~~~~~", "~~~~~~~~~~~~", "~~~~~~~~~~~~", "~~~~~~~~~~~~", "~~~~~~~~~~~~" }, "THE VAPOR HANGS IN THE AIR.", 5.40f },
        // "IT CRYSTALLIZES." (Cluster of crystals)
        { { "~~~~ * ~~~~~", "~~~~~*~~~~~~", "~~~ * ~~~~~~", "~~~~~~~~ * ~", "~~~~~ * ~~~~" }, "IT CRYSTALLIZES.", 6.00f },
        { { "~~~ *** ~~~~", "~~~~***~~~~~", "~~~ *** ~~~~", "~~~~~~~ *** ", "~~~~ *** ~~~" }, "IT CRYSTALLIZES.", 6.60f },
        { { "~~  /\\\\  ~~~", "~~~//  \\\\~~~", "~~ \\\\  //~~~", "~~~~\\\\//~~~~", "~~~~ \\/ ~~~~" }, "IT CRYSTALLIZES.", 7.20f },
        { { "    /\\\\     ", "   //  \\\\   ", "   \\\\  //   ", "    \\\\//    ", "     \\/     " }, "IT CRYSTALLIZES.", 7.80f },
        { { "     /\\     ", "   //  \\\\   ", "  /\\\\  //\\  ", "  \\//  \\\\/  ", "   \\\\  //   " }, "IT CRYSTALLIZES.", 8.40f },
        // "THE AIR IS DEAD." (Ice and icicles)
        { { " \\/ | \\/ | \\", "  | |  | |  ", "  |    |    ", "            ", "            " }, "THE AIR IS DEAD.", 9.00f },
        { { " \\/ | \\/ | \\", "  | |  | |  ", "  | |  | |  ", "    |       ", "            " }, "THE AIR IS DEAD.", 9.60f },
        { { " \\/ | \\/ | \\", " \\| | \\| | \\", "  | |  | |  ", "  | |  |    ", "    |       " }, "THE AIR IS DEAD.", 10.20f },
        { { " \\/ | \\/ | \\", " \\| | \\| | \\", " \\| | \\| | \\", "  | |  | |  ", "  |    |    " }, "THE AIR IS DEAD.", 10.80f },
        { { " \\/ | \\/ | \\", " \\| | \\| | \\", " \\| | \\| | \\", " \\| | \\| | \\", " \\| | \\| | \\" }, "THE AIR IS DEAD.", 11.40f },
        // "YOU ARE FREEZING." (Frozen block)
        { { " \\/ | \\/ | \\", " \\| | \\| | \\", " \\| | \\| | \\", " [########] ", " [########] " }, "YOU ARE FREEZING.", 12.00f },
        { { " \\/ | \\/ | \\", " \\| | \\| | \\", " [########] ", " [########] ", " [########] " }, "YOU ARE FREEZING.", 12.60f },
        { { " \\/ | \\/ | \\", " [########] ", " [########] ", " [########] ", " [########] " }, "YOU ARE FREEZING.", 13.20f },
        { { " [########] ", " [########] ", " [########] ", " [########] ", " [########] " }, "YOU ARE FREEZING.", 13.80f },
        { { " [========] ", " [========] ", " [========] ", " [========] ", " [========] " }, "YOU ARE FREEZING.", 14.40f }
    };

    // 
    // Choice 1: "IT'S SO COLD I CAN'T MOVE"
    // Theme: Walk/Fall -> Neuron firing -> Flexing arm -> Mountain -> Mummy
    // 
    g_mindTrapMovies[0][1] = {
        // "YOU COMMAND YOUR LEGS." (Person walking then falling)
        { { "      O     ", "     /|\\    ", "     / \\    ", "            ", "            " }, "YOU COMMAND YOUR LEGS.", 0.00f },
        { { "      O     ", "     /|     ", "     / \\    ", "            ", "            " }, "YOU COMMAND YOUR LEGS.", 0.60f },
        { { "            ", "     \\O/    ", "      |     ", "     / \\    ", "            " }, "YOU COMMAND YOUR LEGS.", 1.20f },
        { { "            ", "            ", "    __O__   ", "      |     ", "     / \\    " }, "YOU COMMAND YOUR LEGS.", 1.80f },
        { { "            ", "            ", "            ", "    __O__   ", "___/\\_/\\____" }, "YOU COMMAND YOUR LEGS.", 2.40f },
        // "THE SYNAPSE FIRES." (Neuron firing)
        { { "            ", "      O     ", "     / \\    ", "            ", "            " }, "THE SYNAPSE FIRES.", 3.00f },
        { { "      |     ", "    - O -   ", "      |     ", "            ", "            " }, "THE SYNAPSE FIRES.", 3.60f },
        { { "    \\ | /   ", "   -- O --  ", "    / | \\   ", "            ", "            " }, "THE SYNAPSE FIRES.", 4.20f },
        { { " * \\ | /  *", " * -- O -- *", " * / | \\  *", "            ", "            " }, "THE SYNAPSE FIRES.", 4.80f },
        { { " ** \\ | / **", " **-- O --**", " ** / | \\ **", "   * * ", "            " }, "THE SYNAPSE FIRES.", 5.40f },
        // "THE MUSCLE STRAINS." (Person flexing their arm)
        { { "      O     ", "      |     ", "     / \\    ", "            ", "            " }, "THE MUSCLE STRAINS.", 6.00f },
        { { "      O_    ", "     /|     ", "     / \\    ", "            ", "            " }, "THE MUSCLE STRAINS.", 6.60f },
        { { "     _O_    ", "    / | \\   ", "     / \\    ", "            ", "            " }, "THE MUSCLE STRAINS.", 7.20f },
        { { "    \\_O_    ", "      | \\   ", "     / \\    ", "            ", "            " }, "THE MUSCLE STRAINS.", 7.80f },
        { { "    \\_O_/   ", "      |     ", "     / \\    ", "            ", "            " }, "THE MUSCLE STRAINS.", 8.40f },
        // "THE STONE DOES NOT LISTEN." (A mountain)
        { { "      ^     ", "            ", "            ", "            ", "            " }, "THE STONE DOES NOT LISTEN.", 9.00f },
        { { "      ^     ", "     / \\    ", "            ", "            ", "            " }, "THE STONE DOES NOT LISTEN.", 9.60f },
        { { "      ^     ", "     / \\    ", "    /   \\   ", "            ", "            " }, "THE STONE DOES NOT LISTEN.", 10.20f },
        { { "      ^     ", "     / \\    ", "    /   \\   ", "   /     \\  ", "            " }, "THE STONE DOES NOT LISTEN.", 10.80f },
        { { "      ^     ", "     / \\    ", "    /   \\   ", "   /     \\  ", "  /_______\\ " }, "THE STONE DOES NOT LISTEN.", 11.40f },
        // "YOU CANNOT MOVE." (A mummy)
        { { "   .----.   ", "  /      \\  ", " |        | ", " |        | ", "  \\      /  " }, "YOU CANNOT MOVE.", 12.00f },
        { { "   .----.   ", "  / =  = \\  ", " | ====== | ", " | ====== | ", "  \\======/  " }, "YOU CANNOT MOVE.", 12.60f },
        { { "   .----.   ", "  / == ==\\  ", " | ====== | ", " | ====== | ", "  \\======/  " }, "YOU CANNOT MOVE.", 13.20f },
        { { "   .====.   ", "  / == ==\\  ", " | ====== | ", " | ====== | ", "  \\======/  " }, "YOU CANNOT MOVE.", 13.80f },
        { { "   [====]   ", "  [======]  ", " [========] ", " [========] ", "  [======]  " }, "YOU CANNOT MOVE.", 14.40f }
    };

    // 
    // Choice 2: "WHO ARE YOU?"
    // Theme: Mirror -> Cave/Dark Void -> Empty Shell -> Creator -> Director
    // 
    g_mindTrapMovies[0][2] = {
        // "LOOK IN THE MIRROR." (A mirror)
        { { "  .------.  ", "  |      |  ", "  |      |  ", "  |      |  ", "  '------'  " }, "LOOK IN THE MIRROR.", 0.00f },
        { { "  .------.  ", "  |  ()  |  ", "  |  /\\  |  ", "  |      |  ", "  '------'  " }, "LOOK IN THE MIRROR.", 0.60f },
        { { "  .------.  ", "  |  ()  |  ", "  | /||\\ |  ", "  |  /\\  |  ", "  '------'  " }, "LOOK IN THE MIRROR.", 1.20f },
        { { "  .------.  ", "  | *()  |  ", "  | /||\\ |  ", "  |  /\\  |  ", "  '------'  " }, "LOOK IN THE MIRROR.", 1.80f },
        { { "  .------.  ", "  | *()* |  ", "  | /||\\ |  ", "  |  /\\  |  ", "  '------'  " }, "LOOK IN THE MIRROR.", 2.40f },
        // "THERE IS NO ONE ELSE." (A cave or dark void)
        { { "   /    \\   ", "  |      |  ", "  |      |  ", "  |      |  ", "   \\____/   " }, "THERE IS NO ONE ELSE.", 3.00f },
        { { "  /      \\  ", " |        | ", " |        | ", " |        | ", "  \\      /  " }, "THERE IS NO ONE ELSE.", 3.60f },
        { { " /        \\ ", "|          |", "|          |", "|          |", " \\        / " }, "THERE IS NO ONE ELSE.", 4.20f },
        { { "            ", "|          |", "|          |", "|          |", "            " }, "THERE IS NO ONE ELSE.", 4.80f },
        { { "            ", "            ", "            ", "            ", "            " }, "THERE IS NO ONE ELSE.", 5.40f },
        // "JUST AN EMPTY SHELL." (An empty shell)
        { { "    __      ", "   /  \\     ", "   \\__/     ", "            ", "            " }, "JUST AN EMPTY SHELL.", 6.00f },
        { { "    _.-.    ", "   /    \\   ", "   \\    /   ", "    '--'    ", "            " }, "JUST AN EMPTY SHELL.", 6.60f },
        { { "   _.-._    ", "  / _.-.\\   ", " | (   ) |  ", "  \\ '-' /   ", "   '-.-'    " }, "JUST AN EMPTY SHELL.", 7.20f },
        { { "   _.-._    ", "  / _.--\\   ", " | (    )|  ", "  \\ '--' /  ", "   '----'   " }, "JUST AN EMPTY SHELL.", 7.80f },
        { { "   _.-._    ", "  /      \\  ", " | (    )|  ", "  \\      /  ", "   '----'   " }, "JUST AN EMPTY SHELL.", 8.40f },
        // "ONLY THE CREATOR." (Face of the creator)
        { { "   .----.   ", "  /      \\  ", " |        | ", " |        | ", "  \\      /  " }, "ONLY THE CREATOR.", 9.00f },
        { { "   .----.   ", "  / o  o \\  ", " |        | ", " |        | ", "  \\      /  " }, "ONLY THE CREATOR.", 9.60f },
        { { "   .----.   ", "  / O  O \\  ", " |   /\\   | ", " |        | ", "  \\      /  " }, "ONLY THE CREATOR.", 10.20f },
        { { "   .----.   ", "  / O  O \\  ", " |   /\\   | ", " |  ----  | ", "  \\      /  " }, "ONLY THE CREATOR.", 10.80f },
        { { "   _----_   ", "  / O  O \\  ", " |   /\\   | ", " |  ====  | ", "  \\      /  " }, "ONLY THE CREATOR.", 11.40f },
        // "ONLY THE DIRECTOR." (The director)
        { { "   ^----^   ", "  / @  @ \\  ", " |   /\\   | ", " |  \\||/  | ", "  \\======/  " }, "ONLY THE DIRECTOR.", 12.00f },
        { { "  <^----^>  ", " </ @  @ \\> ", "<|   /\\   |>", "<|  \\||/  |>", " <\\======/> " }, "ONLY THE DIRECTOR.", 12.60f },
        { { " <<^----^>> ", "<</ @  @ \\>>", "<<|  /\\  |>>", "<<| \\||/ |>>", "<<\\======/>>" }, "ONLY THE DIRECTOR.", 13.20f },
        { { " ||^----^|| ", "||/ @  @ \\||", "|||  /\\  |||", "||| \\||/ |||", "||\\======/||" }, "ONLY THE DIRECTOR.", 13.80f },
        { { " [|^----^|] ", "[/| @  @ |\\]", "[||  /\\  ||]", "[|| \\||/ ||]", "[|\\======/|]" }, "ONLY THE DIRECTOR.", 14.40f }
    };

    // 
        // PHASE 1
        // 
    g_mindTrapMovies[1].resize(3);

    // 
    // Choice 0: "IT IS JUST STONE"
    // Theme: Large stone -> Shattered heart -> Fire -> Dead person -> Particles
    // 
    g_mindTrapMovies[1][0] = {
        // "IT IS JUST STONE." (Large stone)
        {{"   /======\\   ", "  |########|  ", "  |########|  ", "  |########|  ", "   \\======/   "}, "IT IS JUST STONE.", 0.00f},
        {{"   /======\\   ", "  |########|  ", "  |########|  ", "  |########|  ", "   \\======/   "}, "IT IS JUST STONE.", 0.60f},
        {{"   /======\\   ", "  |###/\\###|  ", "  |##/  \\##|  ", "  |########|  ", "   \\======/   "}, "IT IS JUST STONE.", 1.20f},
        {{"   /======\\   ", "  |##/  \\##|  ", "  |#/    \\#|  ", "  |##\\  /##|  ", "   \\======/   "}, "IT IS JUST STONE.", 1.80f},
        {{"   /==  ==\\   ", "  |/      \\|  ", "  /        \\  ", "  \\        /  ", "   \\==  ==/   "}, "IT IS JUST STONE.", 2.40f},
        // "NO BEATING HEART." (Shattered heart)
        {{"   __    __   ", "  /  \\  /  \\  ", "  \\   \\/   /  ", "   \\      /   ", "    \\    /    "}, "NO BEATING HEART.", 3.00f},
        {{"   __    __   ", "  /  \\  /  \\  ", "  \\   \\/   /  ", "   \\      /   ", "    \\    /    "}, "NO BEATING HEART.", 3.60f},
        {{"   __    __   ", "  /  \\// /  \\ ", "  \\   /\\   /  ", "   \\ //   /   ", "    \\/   /    "}, "NO BEATING HEART.", 4.20f},
        {{"   _      _   ", "  / \\    / \\  ", "  \\  \\  /  /  ", "   \\  \\/  /   ", "    \\    /    "}, "NO BEATING HEART.", 4.80f},
        {{"              ", "   /\\    /\\   ", "  /  \\  /  \\  ", "  \\  /  \\  /  ", "   \\/    \\/   "}, "NO BEATING HEART.", 5.40f},
        // "NO WARMTH." (Fire)
        {{"              ", "      ()      ", "     (  )     ", "    (    )    ", "   (______)   "}, "NO WARMTH.", 6.00f},
        {{"      ()      ", "     (  )     ", "    (    )    ", "   (  ()  )   ", "  (________)  "}, "NO WARMTH.", 6.60f},
        {{"      ()      ", "    (    )    ", "   )  ()  (   ", "  (   )(   )  ", "  (________)  "}, "NO WARMTH.", 7.20f},
        {{"     (  )     ", "   )      (   ", "  (  (  )  )  ", "  )  )( (  (  ", "  (________)  "}, "NO WARMTH.", 7.80f},
        {{"    (    )    ", "  )  (  )  (  ", " (  (    )  ) ", " )  )    (  ( ", " (__________) "}, "NO WARMTH.", 8.40f},
        // "NO LIFE." (Dead person)
        {{"      ()      ", "    ) )( (    ", "   ( .--. )   ", "   )/    \\(   ", "   (______)   "}, "NO LIFE.", 9.00f},
        {{"     .--.     ", "    /    \\    ", "    |    |    ", "    \\____/    ", "              "}, "NO LIFE.", 9.60f},
        {{"     .--.     ", "    / x x\\    ", "    | /\\ |    ", "    \\____/    ", "      ||      "}, "NO LIFE.", 10.20f},
        {{"     .--.     ", "    / X X\\    ", "    | /\\ |    ", "    \\_--_/    ", "     /||\\     "}, "NO LIFE.", 10.80f},
        {{"    _.--._    ", "   / X  X \\   ", "   |  /\\  |   ", "   \\_====_/   ", "    //||\\\\    "}, "NO LIFE.", 11.40f},
        // "JUST DEAD MATTER." (Particles fizzling away)
        {{"    _.--._    ", "   / X  X \\   ", "   |  /\\  |   ", "   \\_====_/   ", "    //||\\\\    "}, "JUST DEAD MATTER.", 12.00f},
        {{"    . .. .    ", "   . .  . .   ", "   .  ..  .   ", "   ..    ..   ", "    ..  ..    "}, "JUST DEAD MATTER.", 12.60f},
        {{"      ..      ", "   .      .   ", "      ..      ", "   .      .   ", "      ..      "}, "JUST DEAD MATTER.", 13.20f},
        {{"              ", "      .       ", "              ", "       .      ", "              "}, "JUST DEAD MATTER.", 13.80f},
        {{"              ", "              ", "              ", "              ", "              "}, "JUST DEAD MATTER.", 14.40f}
    };

    // 
    // Choice 1: "IT WAS CREEPY"
    // Theme: Slender figures -> Ear -> Director's face -> Pair of eyes -> Screaming mouth
    // 
    g_mindTrapMovies[1][1] = {
        // "THEY WERE STILL." (Cluster of slender figures)
        {{"   o  o  o    ", "  /|\\/|\\/|\\   ", "  / \\/ \\/ \\   ", "              ", "              "}, "THEY WERE STILL.", 0.00f},
        {{"   o  o  o    ", "  /|\\/|\\/|\\   ", "  / \\/ \\/ \\   ", "              ", "              "}, "THEY WERE STILL.", 0.60f},
        {{"    O  O  O   ", "   /|\\/|\\/|\\  ", "   / \\/ \\/ \\  ", "              ", "              "}, "THEY WERE STILL.", 1.20f},
        {{"  O  O  O  O  ", " /|\\/|\\/|\\/|\\ ", " / \\/ \\/ \\/ \\ ", "              ", "              "}, "THEY WERE STILL.", 1.80f},
        {{" O O O O O O  ", "/|/|/|/|/|/|\\ ", "/|/|/|/|/|/|\\ ", "/ \\/ \\/ \\/ \\/ ", "              "}, "THEY WERE STILL.", 2.40f},
        // "BUT THEY HEARD YOU." (Ear)
        {{"    .-.       ", "   /   |      ", "   | ) |      ", "   \\__/       ", "              "}, "BUT THEY HEARD YOU.", 3.00f},
        {{"     .--.     ", "    /    |    ", "    | )) |    ", "    \\____/    ", "              "}, "BUT THEY HEARD YOU.", 3.60f},
        {{"      .---.   ", "     /     |  ", "     | ))) |  ", "     \\_____/  ", "              "}, "BUT THEY HEARD YOU.", 4.20f},
        {{"      .---.   ", "    //     |  ", "   ((( ))) |  ", "    \\\\_____/  ", "              "}, "BUT THEY HEARD YOU.", 4.80f},
        {{"      .---.   ", "  ////     |  ", " ((((( ))) |  ", "  \\\\\\\\_____/  ", "              "}, "BUT THEY HEARD YOU.", 5.40f},
        // "THEY FELT YOUR PRESENCE." (Director's face)
        {{"    ^----^    ", "   / @  @ \\   ", "   |  /\\  |   ", "   | \\||/ |   ", "    \\====/    "}, "THEY FELT YOUR PRESENCE.", 6.00f},
        {{"    ^----^    ", "   / @  @ \\   ", "   |  /\\  |   ", "   | \\||/ |   ", "    \\====/    "}, "THEY FELT YOUR PRESENCE.", 6.60f},
        {{"   <^----^>   ", "  </ @  @ \\>  ", "  <|  /\\  |>  ", "  <| \\||/ |>  ", "   <\\====/>   "}, "THEY FELT YOUR PRESENCE.", 7.20f},
        {{"  <<^----^>>  ", " <</ @  @ \\>> ", " <<|  /\\  |>> ", " <<| \\||/ |>> ", "  <<\\====/>>  "}, "THEY FELT YOUR PRESENCE.", 7.80f},
        {{" <<<^----^>>> ", "<<</ @  @ \\>>>", "<<<|  /\\  |>>>", "<<<| \\||/ |>>>", " <<<\\====/>>> "}, "THEY FELT YOUR PRESENCE.", 8.40f},
        // "THEY ARE WATCHING." (Pair of eyes)
        {{"    ______    ", "   / @  @ \\   ", "   --------   ", "              ", "              "}, "THEY ARE WATCHING.", 9.00f},
        {{"   ________   ", "  /  @  @  \\  ", "  ----------  ", "              ", "              "}, "THEY ARE WATCHING.", 9.60f},
        {{"  __________  ", " /   @  @   \\ ", " ------------ ", "              ", "              "}, "THEY ARE WATCHING.", 10.20f},
        {{" ____________ ", "/    O  O    \\", "--------------", "              ", "              "}, "THEY ARE WATCHING.", 10.80f},
        {{" ____________ ", "/   (O)(O)   \\", "--------------", "              ", "              "}, "THEY ARE WATCHING.", 11.40f},
        // "THEY ARE SCREAMING." (Screaming mouth)
        {{"    .----.    ", "   /| || |\\   ", "   || || ||   ", "   \\| || |/   ", "    '----'    "}, "THEY ARE SCREAMING.", 12.00f},
        {{"    .----.    ", "   /| || |\\   ", "   || || ||   ", "   \\| || |/   ", "    '----'    "}, "THEY ARE SCREAMING.", 12.60f},
        {{"   _.----._   ", "  / | || | \\  ", "  | | || | |  ", "  \\ | || | /  ", "   '------'   "}, "THEY ARE SCREAMING.", 13.20f},
        {{"  __.----.__  ", " /  | || |  \\ ", " |  | || |  | ", " \\  | || |  / ", "  '--------'  "}, "THEY ARE SCREAMING.", 13.80f},
        {{" ___.----.___ ", "/   | || |   \\", "|   (    )   |", "\\   | || |   /", " '----------' "}, "THEY ARE SCREAMING.", 14.40f}
    };

    // 
    // Choice 2: "I THINK THEY LOOKED VERY REAL"
    // Theme: Scattered bones -> Bottle of solvent -> Blood hardening -> Brain -> Awake eyes
    // 
    g_mindTrapMovies[1][2] = {
        // "FLESH AND BONE." (Scattered bones)
        {{"   .-.  .-.   ", "   | |  \\ \\   ", "  / /   | |   ", "  \\ \\   / /   ", "   '-'  '-'   "}, "FLESH AND BONE.", 0.00f},
        {{"   .-.  .-.   ", "   | |  \\ \\   ", "  / /   | |   ", "  \\ \\   / /   ", "   '-'  '-'   "}, "FLESH AND BONE.", 0.60f},
        {{"    .-.  .-.  ", "    | |  | |  ", "  .-| |  | |  ", "  \\ \\ /  \\ \\  ", "   '-'    '-' "}, "FLESH AND BONE.", 1.20f},
        {{"      .-.     ", "  .-. | | .-. ", "  | |/ /  | | ", "  \\ \\ /   / / ", "   '-'    '-' "}, "FLESH AND BONE.", 1.80f},
        {{"       _      ", "   .-.| |.-.  ", "  / / | | \\ \\ ", "  \\ \\ \\ / / / ", "   '-'   '-'  "}, "FLESH AND BONE.", 2.40f},
        // "THE SOLVENT SEEPS IN." (Bottle of solvent)
        {{"      ||      ", "     /__\\     ", "    |====|    ", "    |====|    ", "     ----     "}, "THE SOLVENT SEEPS IN.", 3.00f},
        {{"      ||      ", "     /__\\     ", "    |====|    ", "    |====|    ", "     ----     "}, "THE SOLVENT SEEPS IN.", 3.60f},
        {{"     \\||/     ", "     /__\\     ", "    |====|    ", "    |====|    ", "     ----     "}, "THE SOLVENT SEEPS IN.", 4.20f},
        {{"    \\\\||//    ", "     /__\\     ", "    |====|    ", "    |====|    ", "     ----     "}, "THE SOLVENT SEEPS IN.", 4.80f},
        {{"   \\\\\\||///   ", "     /__\\     ", "    |    |    ", "    |    |    ", "     ----     "}, "THE SOLVENT SEEPS IN.", 5.40f},
        // "THE VEINS CALCIFY." (Blood hardening)
        {{"      ||      ", "      ||      ", "      ||      ", "     /  \\     ", "    /    \\    "}, "THE VEINS CALCIFY.", 6.00f},
        {{"      ||      ", "      ||      ", "     /||\\     ", "    /||||\\    ", "   /||||||\\   "}, "THE VEINS CALCIFY.", 6.60f},
        {{"      ||      ", "     [##]     ", "    [####]    ", "   [######]   ", "  [########]  "}, "THE VEINS CALCIFY.", 7.20f},
        {{"     [##]     ", "    [####]    ", "   [######]   ", "  [########]  ", " [##########] "}, "THE VEINS CALCIFY.", 7.80f},
        {{"    [####]    ", "   [######]   ", "  [########]  ", " [##########] ", "[############]"}, "THE VEINS CALCIFY.", 8.40f},
        // "THE MIND IS TRAPPED." (Brain)
        {{"    _----_    ", "   (%%%%%%)   ", "   (%%%%%%)   ", "    \\----/    ", "              "}, "THE MIND IS TRAPPED.", 9.00f},
        {{"    _----_    ", "   (%%%%%%)   ", "   (%%%%%%)   ", "    \\----/    ", "              "}, "THE MIND IS TRAPPED.", 9.60f},
        {{"   _------_   ", "  (%%%%%%%%)  ", "  (%%%%%%%%)  ", "   \\------/   ", "              "}, "THE MIND IS TRAPPED.", 10.20f},
        {{"  _--------_  ", " (%%%%%%%%%%) ", " (%%%%%%%%%%) ", "  \\--------/  ", "              "}, "THE MIND IS TRAPPED.", 10.80f},
        {{" _----------_ ", "(%%%%%%%%%%%%)", "(%%%%%%%%%%%%)", " \\----------/ ", "              "}, "THE MIND IS TRAPPED.", 11.40f},
        // "THEY ARE STILL CONSCIOUS." (Pair of awake eyes)
        {{"              ", "   /------\\   ", "   | O  O |   ", "   \\------/   ", "              "}, "THEY ARE STILL CONSCIOUS.", 12.00f},
        {{"              ", "   /------\\   ", "   | O  O |   ", "   \\------/   ", "              "}, "THEY ARE STILL CONSCIOUS.", 12.60f},
        {{"              ", "  /--------\\  ", "  | (O)(O) |  ", "  \\--------/  ", "              "}, "THEY ARE STILL CONSCIOUS.", 13.20f},
        {{"              ", " /----------\\ ", " |  (O)(O)  | ", " \\----------/ ", "              "}, "THEY ARE STILL CONSCIOUS.", 13.80f},
        {{"  /--------\\  ", " /----------\\ ", " | >(O)(O)< | ", " \\----------/ ", "  \\--------/  "}, "THEY ARE STILL CONSCIOUS.", 14.40f}
    };

    // 
    // PHASE 2: THE ETERNITY CLAUSE
    // 
    g_mindTrapMovies[2].resize(3);

    // 
    // Choice 0: "PLEASE NO, I WILL GO MAD"
    // Theme: Clock -> Melting Clock -> Skull -> Melting Skull -> Static Vortex
    // 
    g_mindTrapMovies[2][0] = {
        // "THE YEARS WILL PASS." (Ticking Clock)
        {{"    _----_    ", "   /  ||  \\   ", "  |   o--  |  ", "   \\      /   ", "    -____-    "}, "THE YEARS WILL PASS.", 0.00f},
        {{"    _----_    ", "   /  |   \\   ", "  |   o-   |  ", "   \\      /   ", "    -____-    "}, "THE YEARS WILL PASS.", 0.60f},
        {{"    _----_    ", "   /      \\   ", "  |   o    |  ", "   \\  |   /   ", "    -____-    "}, "THE YEARS WILL PASS.", 1.20f},
        {{"    _----_    ", "   /      \\   ", "  |   o    |  ", "   \\ /    /   ", "    -____-    "}, "THE YEARS WILL PASS.", 1.80f},
        {{"    _----_    ", "   /      \\   ", "  | --o    |  ", "   \\      /   ", "    -____-    "}, "THE YEARS WILL PASS.", 2.40f},
        // "THE SILENCE WILL DEAFEN." (Clock melts into bleeding ear)
        {{"   _------_   ", "  /      | \\  ", " |     )   |  ", "  \\       /   ", "   --____--   "}, "THE SILENCE WILL DEAFEN.", 3.00f},
        {{"   _------_   ", "  /    |   \\  ", " |     )    | ", "  \\   __/  /  ", "   --____--   "}, "THE SILENCE WILL DEAFEN.", 3.60f},
        {{"   _------_   ", "  /    |   \\  ", " |    ))    | ", "  \\  ___/  /  ", "   --____--   "}, "THE SILENCE WILL DEAFEN.", 4.20f},
        {{"    ------    ", "  /    |   \\  ", " |   )))    | ", "  \\  ___/  /  ", "    ||  ||    "}, "THE SILENCE WILL DEAFEN.", 4.80f},
        {{"    ------    ", "  /   ||   \\  ", " |  ))))    | ", "  \\  ___/  /  ", "    ||  ||    "}, "THE SILENCE WILL DEAFEN.", 5.40f},
        // "THE MIND WILL FRACTURE." (Skull Cracking)
        {{"    .----.    ", "   /      \\   ", "  |        |  ", "   \\      /   ", "    '-||-'    "}, "THE MIND WILL FRACTURE.", 6.00f},
        {{"    .----.    ", "   / x  x \\   ", "  |   /\\   |  ", "   \\ '==' /   ", "    '-||-'    "}, "THE MIND WILL FRACTURE.", 6.60f},
        {{"    .----.    ", "   / X  X \\   ", "  |   /\\   |  ", "   \\ '==' /   ", "    '-||-'    "}, "THE MIND WILL FRACTURE.", 7.20f},
        {{"    ._--_.    ", "   / X  X \\   ", "  |   /\\   |  ", "  >\\ '==' /<  ", "  >_--||--_<  "}, "THE MIND WILL FRACTURE.", 7.80f},
        {{"    ._--_.    ", "  </ X  X \\>  ", " <|   /\\   |> ", "  >\\ '==' /<  ", "  >_--||--_<  "}, "THE MIND WILL FRACTURE.", 8.40f},
        // "SANITY WILL SLIP." (Static Bursting from skull)
        {{"  %#_----_%#  ", " %#/ %&% \\#%  ", " &(# %XX# )&  ", "  %\\ &%#$ /%  ", "  #%\\____/#%  "}, "SANITY WILL SLIP.", 9.00f},
        {{"  #%_----_#%  ", " #%\\ %&% /%#  ", " %(& #XX% &)  ", "  /% $#%& \\%  ", "  %#\\____/%#  "}, "SANITY WILL SLIP.", 9.60f},
        {{"  %#_----_%#  ", " %#/ %&% \\#%  ", " &(# %XX# )&  ", "  %\\ &%#$ /%  ", "  #%\\____/#%  "}, "SANITY WILL SLIP.", 10.20f},
        {{"  #%_----_#%  ", " #%\\ %&% /%#  ", " %(& #XX% &)  ", "  /% $#%& \\%  ", "  %#\\____/%#  "}, "SANITY WILL SLIP.", 10.80f},
        {{"  %#_----_%#  ", " %#/ %&% \\#%  ", " &(# %XX# )&  ", "  %\\ &%#$ /%  ", "  #%\\____/#%  "}, "SANITY WILL SLIP.", 11.40f},
        // "ONLY MADNESS REMAINS." (Vortex)
        {{"   %&%#$#&%   ", "  #%$%&%#$#%  ", " %&#$#%&%#$&  ", "  %#$&%#$#&%  ", "   $#%&%#$#   "}, "ONLY MADNESS REMAINS.", 12.00f},
        {{"   #$#%&%#$   ", "  %#$&%#$#&%  ", " &%#$#%&%#$&  ", "  #%$%&%#$#%  ", "   %&%#$#&%   "}, "ONLY MADNESS REMAINS.", 12.60f},
        {{"   %&%#$#&%   ", "  #%$%&%#$#%  ", " %&#$#%&%#$&  ", "  %#$&%#$#&%  ", "   $#%&%#$#   "}, "ONLY MADNESS REMAINS.", 13.20f},
        {{"   #$#%&%#$   ", "  %#$&%#$#&%  ", " &%#$#%&%#$&  ", "  #%$%&%#$#%  ", "   %&%#$#&%   "}, "ONLY MADNESS REMAINS.", 13.80f},
        {{"   %&%#$#&%   ", "  #%$%&%#$#%  ", " %&#$#%&%#$&  ", "  %#$&%#$#&%  ", "   $#%&%#$#   "}, "ONLY MADNESS REMAINS.", 14.40f}
    };

    // 
    // Choice 1: "PLEASE LET ME DIE"
    // Theme: Tombstone -> Chains -> Flesh poking through -> Fusing to Metal -> Mechanical Eye
    // 
    g_mindTrapMovies[2][1] = {
        // "DEATH IS A MERCY." (Tombstone)
        {{"    .----.    ", "   /  RIP \\   ", "   |      |   ", "   |      |   ", "  _|_----_|_  "}, "DEATH IS A MERCY.", 0.00f},
        {{"    .----.    ", "   /  RIP \\   ", "   |      |   ", "   |      |   ", "  _|_----_|_  "}, "DEATH IS A MERCY.", 0.60f},
        {{"    .----.    ", "   /  RIP \\   ", "   |      |   ", "   |      |   ", "  _|_----_|_  "}, "DEATH IS A MERCY.", 1.20f},
        {{"    .----.    ", "   /  RIP \\   ", "   |      |   ", "   |      |   ", "  _|_----_|_  "}, "DEATH IS A MERCY.", 1.80f},
        {{"    .----.    ", "   /  RIP \\   ", "   |      |   ", "   |      |   ", "  _|_----_|_  "}, "DEATH IS A MERCY.", 2.40f},
        // "A MERCY DENIED TO YOU." (Chains wrapping)
        {{"    .----.    ", "  x/  RIP \\x  ", "  |      |   ", "   |      |   ", "  _|_----_|_  "}, "A MERCY DENIED TO YOU.", 3.00f},
        {{"    .----.    ", "  x/  RIP \\x  ", "  |xxxxxx|   ", "   |      |   ", "  _|_----_|_  "}, "A MERCY DENIED TO YOU.", 3.60f},
        {{"    .----.    ", "  x/  RIP \\x  ", "  |xxxxxx|   ", "  x|xxxx|x   ", "  _|_----_|_  "}, "A MERCY DENIED TO YOU.", 4.20f},
        {{"    .----.    ", "  x/  RIP \\x  ", "  |xxxxxx|   ", "  x|xxxx|x   ", " x_|_----_|_x "}, "A MERCY DENIED TO YOU.", 4.80f},
        {{"   x.----.x   ", "  x/  RIP \\x  ", "  |xxxxxx|   ", "  x|xxxx|x   ", " x_|_----_|_x "}, "A MERCY DENIED TO YOU.", 5.40f},
        // "THE FLESH CANNOT DECAY." (Screaming Face pushing thru)
        {{"   x.----.x   ", "  x/ (  ) \\x  ", "  |xxxxxx|   ", "  x|xxxx|x   ", " x_|_----_|_x "}, "YOU CANNOT DECAY.", 6.00f},
        {{"   x.----.x   ", "  x/ (OO) \\x  ", "  |xxxxxx|   ", "  x|xxxx|x   ", " x_|_----_|_x "}, "YOU CANNOT DECAY.", 6.60f},
        {{"   x.----.x   ", "  x/ (OO) \\x  ", "  |xx || xx|  ", "  x|xxxx|x   ", " x_|_----_|_x "}, "YOU CANNOT DECAY.", 7.20f},
        {{"   x.----.x   ", "  x/ (OO) \\x  ", "  |xx || xx|  ", "  x| == |x   ", " x_|_----_|_x "}, "YOU CANNOT DECAY.", 7.80f},
        {{"   x.----.x   ", "  x/ (OO) \\x  ", "  |xx || xx|  ", "  x| ==== |x  ", " x_|_----_|_x "}, "YOU CANNOT DECAY.", 8.40f},
        // "YOU WILL NEVER ROT." (Fusing with Metal)
        {{"   /-.--.-\\   ", "  |/ (OO) \\|  ", "  |x  ||  x|  ", "  x| ==== |x  ", " x_|_----_|_x "}, "YOU WILL NEVER ROT.", 9.00f},
        {{"   /-\\--/-\\   ", "  |  (OO)  |  ", "  |   ||   |  ", "   | ==== |   ", "  _|_----_|_  "}, "YOU WILL NEVER ROT.", 9.60f},
        {{"   /-\\--/-\\   ", "  |  (OO)  |  ", "  | #[__]# |  ", "   | ==== |   ", "  _|_----_|_  "}, "YOU WILL NEVER ROT.", 10.20f},
        {{"   /-\\--/-\\   ", "  |  (OO)  |  ", "  | #[__]# |  ", "   \\--\\/--/   ", "   [######]   "}, "YOU WILL NEVER ROT.", 10.80f},
        {{"  [========]  ", "  |  (OO)  |  ", "  | #[__]# |  ", "   \\--\\/--/   ", "   [######]   "}, "YOU WILL NEVER ROT.", 11.40f},
        // "YOU WILL SUFFER FOREVER." (Mechanical Eye)
        {{"  [========]  ", "  [  /--\\  ]  ", "  [ <(  )> ]  ", "  [  \\--/  ]  ", "  [========]  "}, "YOU WILL SUFFER FOREVER.", 12.00f},
        {{"  [========]  ", "  [  /--\\  ]  ", "  [ <(())> ]  ", "  [  \\--/  ]  ", "  [========]  "}, "YOU WILL SUFFER FOREVER.", 12.60f},
        {{"  [========]  ", "  [  ----  ]  ", "  [ ------ ]  ", "  [  ----  ]  ", "  [========]  "}, "YOU WILL SUFFER FOREVER.", 13.20f},
        {{"  [========]  ", "  [  /--\\  ]  ", "  [ <(())> ]  ", "  [  \\--/  ]  ", "  [========]  "}, "YOU WILL SUFFER FOREVER.", 13.80f},
        {{"  [========]  ", "  [  /--\\  ]  ", "  [ <(())> ]  ", "  [  \\--/  ]  ", "  [========]  "}, "YOU WILL SUFFER FOREVER.", 14.40f}
    };

    // 
    // Choice 2: "ME? I WON'T BE ALIVE IN CENTURIES"
    // Theme: Body -> Ash -> Brain/Spine -> Pedestal -> Monolith Face
    // 
    g_mindTrapMovies[2][2] = {
        // "YOUR BODY WILL END." (Body)
        {{"      ()      ", "     /||\\     ", "      ||      ", "     /  \\     ", "    /    \\    "}, "YOUR BODY WILL END.", 0.00f},
        {{"      ()      ", "     /||\\     ", "      ||      ", "     /  \\     ", "    /    \\    "}, "YOUR BODY WILL END.", 0.60f},
        {{"      ()      ", "     /||\\     ", "      ||      ", "     /  \\     ", "    /    \\    "}, "YOUR BODY WILL END.", 1.20f},
        {{"      ()      ", "     /||\\     ", "      ||      ", "     /  \\     ", "    /    \\    "}, "YOUR BODY WILL END.", 1.80f},
        {{"      ()      ", "     /||\\     ", "      ||      ", "     /  \\     ", "              "}, "YOUR BODY WILL END.", 2.40f},
        // "BUT THE STONE ENDURES." (Ash)
        {{"      ()      ", "     /||\\     ", "      ||      ", "              ", "              "}, "BUT THE STONE ENDURES.", 3.00f},
        {{"      ()      ", "      ||      ", "              ", "              ", "              "}, "BUT THE STONE ENDURES.", 3.60f},
        {{"      .       ", "     . ..     ", "      ..      ", "     .  .     ", "              "}, "BUT THE STONE ENDURES.", 4.20f},
        {{"      ..      ", "     .  .     ", "    .    .    ", "   .      .   ", "  .        .  "}, "BUT THE STONE ENDURES.", 4.80f},
        {{"              ", "      ..      ", "     .  .     ", "    .    .    ", "   .      .   "}, "BUT THE STONE ENDURES.", 5.40f},
        // "IT PRESERVES THE NERVES." (Glowing Nerves)
        {{"    (%%%%)    ", "              ", "              ", "              ", "              "}, "IT PRESERVES THE NERVES.", 6.00f},
        {{"    (%%%%)    ", "      ||      ", "              ", "              ", "              "}, "IT PRESERVES THE NERVES.", 6.60f},
        {{"    (%%%%)    ", "      ||      ", "     =||=     ", "              ", "              "}, "IT PRESERVES THE NERVES.", 7.20f},
        {{"    (%%%%)    ", "      ||      ", "     =||=     ", "      ||      ", "              "}, "IT PRESERVES THE NERVES.", 7.80f},
        {{"    (%%%%)    ", "      ||      ", "     =||=     ", "      ||      ", "     /  \\     "}, "IT PRESERVES THE NERVES.", 8.40f},
        // "IT HOLDS THE SOUL." (Pedestal)
        {{"    (%%%%)    ", "      ||      ", "     =||=     ", "      ||      ", "   [======]   "}, "IT HOLDS THE SOUL.", 9.00f},
        {{"    (%%%%)    ", "      ||      ", "     =||=     ", "   [======]   ", "   [======]   "}, "IT HOLDS THE SOUL.", 9.60f},
        {{"    (%%%%)    ", "      ||      ", "   [======]   ", "   [======]   ", "   [======]   "}, "IT HOLDS THE SOUL.", 10.20f},
        {{"    (%%%%)    ", "   [======]   ", "   [======]   ", "   [======]   ", "   [======]   "}, "IT HOLDS THE SOUL.", 10.80f},
        {{"   [======]   ", "   [======]   ", "   [======]   ", "   [======]   ", "   [======]   "}, "IT HOLDS THE SOUL.", 11.40f},
        // "ETERNITY IS MANDATORY." (Monolith Face)
        {{"   [======]   ", "   [ O  O ]   ", "   [======]   ", "   [======]   ", "   [======]   "}, "ETERNITY IS MANDATORY.", 12.00f},
        {{"   [======]   ", "   [ O  O ]   ", "   [  /\\  ]   ", "   [======]   ", "   [======]   "}, "ETERNITY IS MANDATORY.", 12.60f},
        {{"   [======]   ", "   [ O  O ]   ", "   [  /\\  ]   ", "   [ ==== ]   ", "   [======]   "}, "ETERNITY IS MANDATORY.", 13.20f},
        {{"   [======]   ", "   [ O  O ]   ", "   [  /\\  ]   ", "   [ ==== ]   ", "   [======]   "}, "ETERNITY IS MANDATORY.", 13.80f},
        {{"   [======]   ", "   [ O  O ]   ", "   [  /\\  ]   ", "   [ ==== ]   ", "   [======]   "}, "ETERNITY IS MANDATORY.", 14.40f}
    };


    // 
    // PHASE 3: REVEAL 1 - THE ARCHITECT
    // 
    g_mindTrapMovies[3].resize(3);

    // 
    // Choice 0: "WHO DID THIS TO ME?!"
    // Theme: Papers -> Finger pointing -> Bloody Handprint -> Palm Lines -> Player Face
    // 
    g_mindTrapMovies[3][0] = {
        // "HE GAVE THE ORDERS." (Papers)
        {{"  +--------+  ", "  | ORDERS |  ", "  |        |  ", "  |        |  ", "  +--------+  "}, "HE GAVE THE ORDERS.", 0.00f},
        {{"  +--------+  ", "  | ORDERS |  ", "  |      X |  ", "  |        |  ", "  +--------+  "}, "HE GAVE THE ORDERS.", 0.60f},
        {{"  +--------+  ", "  | ORDERS |  ", "  | SIGNED |  ", "  |      X |  ", "  +--------+  "}, "HE GAVE THE ORDERS.", 1.20f},
        {{"  +--------+  ", "  | ORDERS |  ", "  | SIGNED |  ", "  |      X |  ", "  +--------+  "}, "HE GAVE THE ORDERS.", 1.80f},
        {{"  +--------+  ", "  | ORDERS |  ", "  | SIGNED |  ", "  |      X |  ", "  +--------+  "}, "HE GAVE THE ORDERS.", 2.40f},
        // "HE DESIGNED THE VATS." (Finger Pointing)
        {{"              ", "              ", "  ===)   )>>  ", "              ", "              "}, "HE DESIGNED THE VATS.", 3.00f},
        {{"              ", "              ", "  ===)   )>>  ", "              ", "              "}, "HE DESIGNED THE VATS.", 3.60f},
        {{"              ", "              ", "  ===)   )>>  ", "              ", "              "}, "HE DESIGNED THE VATS.", 4.20f},
        {{"              ", "              ", "  ===)   )>>  ", "              ", "              "}, "HE DESIGNED THE VATS.", 4.80f},
        {{"              ", "              ", "  ===)   )>>  ", "              ", "              "}, "HE DESIGNED THE VATS.", 5.40f},
        // "HE SIGNED THE PAPERS." (Bloody Handprint)
        {{"    \\| |/     ", "   -- | --    ", "   -- | --    ", "    / | \\     ", "     \\|/      "}, "HE SIGNED THE PAPERS.", 6.00f},
        {{"    \\| |/     ", "   -- | --    ", "   -- | --    ", "    / | \\     ", "     \\|/      "}, "HE SIGNED THE PAPERS.", 6.60f},
        {{"    \\| |/     ", "   -- | --    ", "   -- | --    ", "    / | \\     ", "     \\|/      "}, "HE SIGNED THE PAPERS.", 7.20f},
        {{"    \\| |/     ", "   -- | --    ", "   -- | --    ", "    / | \\     ", "     \\|/      "}, "HE SIGNED THE PAPERS.", 7.80f},
        {{"    \\| |/     ", "   -- | --    ", "   -- | --    ", "    / | \\     ", "     \\|/      "}, "HE SIGNED THE PAPERS.", 8.40f},
        // "LOOK CLOSER." (Palm Lines)
        {{"   \\ \\ | / /  ", "  ---\\ | /--- ", "  ---- | ---- ", "  ---/ | \\--- ", "   / / | \\ \\  "}, "LOOK CLOSER.", 9.00f},
        {{"   \\ \\ | / /  ", "  ---\\ | /--- ", "  ---- | ---- ", "  ---/ | \\--- ", "   / / | \\ \\  "}, "LOOK CLOSER.", 9.60f},
        {{"   \\ \\ | / /  ", "  ---\\ | /--- ", "  ---- | ---- ", "  ---/ | \\--- ", "   / / | \\ \\  "}, "LOOK CLOSER.", 10.20f},
        {{"   \\ \\ | / /  ", "  ---\\ | /--- ", "  ---- | ---- ", "  ---/ | \\--- ", "   / / | \\ \\  "}, "LOOK CLOSER.", 10.80f},
        {{"   \\ \\ | / /  ", "  ---\\ | /--- ", "  ---- | ---- ", "  ---/ | \\--- ", "   / / | \\ \\  "}, "LOOK CLOSER.", 11.40f},
        // "IT WAS YOUR HAND." (Player Face)
        {{"    .----.    ", "   / o  o \\   ", "   |  /\\  |   ", "   | ==== |   ", "    \\----/    "}, "IT WAS YOUR HAND.", 12.00f},
        {{"    .----.    ", "   / o  o \\   ", "   |  /\\  |   ", "   | ==== |   ", "    \\----/    "}, "IT WAS YOUR HAND.", 12.60f},
        {{"    .----.    ", "   / o  o \\   ", "   |  /\\  |   ", "   | ==== |   ", "    \\----/    "}, "IT WAS YOUR HAND.", 13.20f},
        {{"    .----.    ", "   / o  o \\   ", "   |  /\\  |   ", "   | ==== |   ", "    \\----/    "}, "IT WAS YOUR HAND.", 13.80f},
        {{"    .----.    ", "   / o  o \\   ", "   |  /\\  |   ", "   | ==== |   ", "    \\----/    "}, "IT WAS YOUR HAND.", 14.40f}
    };

    // 
    // Choice 1: "I DEMAND TO SEE THE PERSON IN CHARGE"
    // Theme: Jaws -> Crown -> Melting Crown -> Puddle -> Player Reflection
    // 
    g_mindTrapMovies[3][1] = {
        // "A MONSTER PROWLS." (Jaws)
        {{"   /\\/\\/\\/\\   ", "   \\      /   ", "   /      \\   ", "   \\/\\/\\/\\/   ", "              "}, "A MONSTER PROWLS.", 0.00f},
        {{"   /\\/\\/\\/\\   ", "   \\      /   ", "   /      \\   ", "   \\/\\/\\/\\/   ", "              "}, "A MONSTER PROWLS.", 0.60f},
        {{"   /\\/\\/\\/\\   ", "   \\      /   ", "   /      \\   ", "   \\/\\/\\/\\/   ", "              "}, "A MONSTER PROWLS.", 1.20f},
        {{"   /\\/\\/\\/\\   ", "   \\      /   ", "   /      \\   ", "   \\/\\/\\/\\/   ", "              "}, "A MONSTER PROWLS.", 1.80f},
        {{"   /\\/\\/\\/\\   ", "   \\      /   ", "   /      \\   ", "   \\/\\/\\/\\/   ", "              "}, "A MONSTER PROWLS.", 2.40f},
        // "A BEAST OF NO EMPATHY." (Crown)
        {{"  |\\|/||\\|/|  ", "  |        |  ", "   \\      /   ", "    ------    ", "              "}, "A BEAST OF NO EMPATHY.", 3.00f},
        {{"  |\\|/||\\|/|  ", "  |        |  ", "   \\      /   ", "    ------    ", "              "}, "A BEAST OF NO EMPATHY.", 3.60f},
        {{"  |\\|/||\\|/|  ", "  |        |  ", "   \\      /   ", "    ------    ", "              "}, "A BEAST OF NO EMPATHY.", 4.20f},
        {{"  |\\|/||\\|/|  ", "  |        |  ", "   \\      /   ", "    ------    ", "              "}, "A BEAST OF NO EMPATHY.", 4.80f},
        {{"  |\\|/||\\|/|  ", "  |        |  ", "   \\      /   ", "    ------    ", "              "}, "A BEAST OF NO EMPATHY.", 5.40f},
        // "A CREATURE OF PURE EGO." (Crown Melting)
        {{"  |\\ /  \\ /|  ", "  |   \\/   |  ", "   \\      /   ", "    ~~~~~~    ", "  ~~~~~~~~~~  "}, "A CREATURE OF PURE EGO.", 6.00f},
        {{"  |\\ /  \\ /|  ", "  |   \\/   |  ", "   \\      /   ", "    ~~~~~~    ", "  ~~~~~~~~~~  "}, "A CREATURE OF PURE EGO.", 6.60f},
        {{"  |\\ /  \\ /|  ", "  |   \\/   |  ", "   \\      /   ", "    ~~~~~~    ", "  ~~~~~~~~~~  "}, "A CREATURE OF PURE EGO.", 7.20f},
        {{"  |\\ /  \\ /|  ", "  |   \\/   |  ", "   \\      /   ", "    ~~~~~~    ", "  ~~~~~~~~~~  "}, "A CREATURE OF PURE EGO.", 7.80f},
        {{"  |\\ /  \\ /|  ", "  |   \\/   |  ", "   \\      /   ", "    ~~~~~~    ", "  ~~~~~~~~~~  "}, "A CREATURE OF PURE EGO.", 8.40f},
        // "LOOK IN THE REFLECTION." (Puddle)
        {{"              ", "  ~~~~~~~~~~  ", " ~~~~~~~~~~~~ ", "  ~~~~~~~~~~  ", "              "}, "LOOK IN THE REFLECTION.", 9.00f},
        {{"              ", "  ~~~~~~~~~~  ", " ~~~~~~~~~~~~ ", "  ~~~~~~~~~~  ", "              "}, "LOOK IN THE REFLECTION.", 9.60f},
        {{"              ", "  ~~~~~~~~~~  ", " ~~~~~~~~~~~~ ", "  ~~~~~~~~~~  ", "              "}, "LOOK IN THE REFLECTION.", 10.20f},
        {{"              ", "  ~~~~~~~~~~  ", " ~~~~~~~~~~~~ ", "  ~~~~~~~~~~  ", "              "}, "LOOK IN THE REFLECTION.", 10.80f},
        {{"              ", "  ~~~~~~~~~~  ", " ~~~~~~~~~~~~ ", "  ~~~~~~~~~~  ", "              "}, "LOOK IN THE REFLECTION.", 11.40f},
        // "YOU ARE THE MONSTER." (Perfect Mirror Face)
        {{"    .----.    ", "   / O  O \\   ", "   |  /\\  |   ", "   | .||. |   ", "    \\----/    "}, "YOU ARE THE MONSTER.", 12.00f},
        {{"    .----.    ", "   / O  O \\   ", "   |  /\\  |   ", "   | .||. |   ", "    \\----/    "}, "YOU ARE THE MONSTER.", 12.60f},
        {{"    .----.    ", "   / O  O \\   ", "   |  /\\  |   ", "   | .||. |   ", "    \\----/    "}, "YOU ARE THE MONSTER.", 13.20f},
        {{"    .----.    ", "   / O  O \\   ", "   |  /\\  |   ", "   | .||. |   ", "    \\----/    "}, "YOU ARE THE MONSTER.", 13.80f},
        {{"    .----.    ", "   / O  O \\   ", "   |  /\\  |   ", "   | .||. |   ", "    \\----/    "}, "YOU ARE THE MONSTER.", 14.40f}
    };

    // 
    // Choice 2: "THIS MUST BE A MISTAKE"
    // Theme: Flask -> Gears -> Swallowing Throat -> Body Calcifying -> Museum Display
    // 
    g_mindTrapMovies[3][2] = {
        // "YOU POURED THE SOLVENT." (Flask)
        {{"     |  |     ", "    /____\\    ", "   /      \\   ", "  |========|  ", "   \\______/   "}, "YOU POURED THE SOLVENT.", 0.00f},
        {{"     |  |     ", "    /____\\    ", "   /      \\   ", "  |========|  ", "   \\______/   "}, "YOU POURED THE SOLVENT.", 0.60f},
        {{"     |  |     ", "    /____\\    ", "   /      \\   ", "  |========|  ", "   \\______/   "}, "YOU POURED THE SOLVENT.", 1.20f},
        {{"     |  |     ", "    /____\\    ", "   /      \\   ", "  |========|  ", "   \\______/   "}, "YOU POURED THE SOLVENT.", 1.80f},
        {{"     |  |     ", "    /____\\    ", "   /      \\   ", "  |========|  ", "   \\______/   "}, "YOU POURED THE SOLVENT.", 2.40f},
        // "YOU SEALED THEIR FATES." (Gears)
        {{"    _-\\/-_    ", "   /      \\   ", "   | (()) |   ", "   \\      /   ", "    ^-/\\-^    "}, "YOU SEALED THEIR FATES.", 3.00f},
        {{"    _-\\/-_    ", "   /      \\   ", "   | (()) |   ", "   \\      /   ", "    ^-/\\-^    "}, "YOU SEALED THEIR FATES.", 3.60f},
        {{"    _-\\/-_    ", "   /      \\   ", "   | (()) |   ", "   \\      /   ", "    ^-/\\-^    "}, "YOU SEALED THEIR FATES.", 4.20f},
        {{"    _-\\/-_    ", "   /      \\   ", "   | (()) |   ", "   \\      /   ", "    ^-/\\-^    "}, "YOU SEALED THEIR FATES.", 4.80f},
        {{"    _-\\/-_    ", "   /      \\   ", "   | (()) |   ", "   \\      /   ", "    ^-/\\-^    "}, "YOU SEALED THEIR FATES.", 5.40f},
        // "NOW YOU TASTE YOUR WORK." (Swallowing Throat)
        {{"    \\ || /    ", "     \\||/     ", "     |##|     ", "    / || \\    ", "   /  ||  \\   "}, "NOW YOU TASTE YOUR WORK.", 6.00f},
        {{"    \\ || /    ", "     \\||/     ", "     |##|     ", "    / || \\    ", "   /  ||  \\   "}, "NOW YOU TASTE YOUR WORK.", 6.60f},
        {{"    \\ || /    ", "     \\||/     ", "     |##|     ", "    / || \\    ", "   /  ||  \\   "}, "NOW YOU TASTE YOUR WORK.", 7.20f},
        {{"    \\ || /    ", "     \\||/     ", "     |##|     ", "    / || \\    ", "   /  ||  \\   "}, "NOW YOU TASTE YOUR WORK.", 7.80f},
        {{"    \\ || /    ", "     \\||/     ", "     |##|     ", "    / || \\    ", "   /  ||  \\   "}, "NOW YOU TASTE YOUR WORK.", 8.40f},
        // "THE CREATOR BECOMES..." (Body Calcifying)
        {{"      ()      ", "    /[##]\\    ", "    [####]    ", "    /[##]\\    ", "   / [##] \\   "}, "THE CREATOR BECOMES...", 9.00f},
        {{"      ()      ", "    /[##]\\    ", "    [####]    ", "    /[##]\\    ", "   / [##] \\   "}, "THE CREATOR BECOMES...", 9.60f},
        {{"      ()      ", "    /[##]\\    ", "    [####]    ", "    /[##]\\    ", "   / [##] \\   "}, "THE CREATOR BECOMES...", 10.20f},
        {{"      ()      ", "    /[##]\\    ", "    [####]    ", "    /[##]\\    ", "   / [##] \\   "}, "THE CREATOR BECOMES...", 10.80f},
        {{"      ()      ", "    /[##]\\    ", "    [####]    ", "    /[##]\\    ", "   / [##] \\   "}, "THE CREATOR BECOMES...", 11.40f},
        // "THE FINAL EXHIBIT." (Museum Display)
        {{"  [========]  ", "  | [####] |  ", "  | [####] |  ", "  | [####] |  ", "  [========]  "}, "THE FINAL EXHIBIT.", 12.00f},
        {{"  [========]  ", "  | [####] |  ", "  | [####] |  ", "  | [####] |  ", "  [========]  "}, "THE FINAL EXHIBIT.", 12.60f},
        {{"  [========]  ", "  | [####] |  ", "  | [####] |  ", "  | [####] |  ", "  [========]  "}, "THE FINAL EXHIBIT.", 13.20f},
        {{"  [========]  ", "  | [####] |  ", "  | [####] |  ", "  | [####] |  ", "  [========]  "}, "THE FINAL EXHIBIT.", 13.80f},
        {{"  [========]  ", "  | [####] |  ", "  | [####] |  ", "  | [####] |  ", "  [========]  "}, "THE FINAL EXHIBIT.", 14.40f}
    };


    // 
    // PHASE 4: REVEAL 2 - THE MASTERPIECE
    // 
    g_mindTrapMovies[4].resize(3);

    // 
    // Choice 0: "NO! I REFUSE!"
    // Theme: Fist -> Stuck in flesh wall -> Veins -> Swallowed -> Wall of trapped souls
    // 
    g_mindTrapMovies[4][0] = {
        // "DENIAL IS NATURAL." (Fist)
        {{"    _..._     ", "   (_____)    ", "   (_____)    ", "   (_____)    ", "    |||||     "}, "DENIAL IS NATURAL.", 0.00f},
        {{"    _..._     ", "   (_____)    ", "   (_____)    ", "   (_____)    ", "    |||||     "}, "DENIAL IS NATURAL.", 0.60f},
        {{"    _..._     ", "   (_____)    ", "   (_____)    ", "   (_____)    ", "    |||||     "}, "DENIAL IS NATURAL.", 1.20f},
        {{"    _..._     ", "   (_____)    ", "   (_____)    ", "   (_____)    ", "    |||||     "}, "DENIAL IS NATURAL.", 1.80f},
        {{"    _..._     ", "   (_____)    ", "   (_____)    ", "   (_____)    ", "    |||||     "}, "DENIAL IS NATURAL.", 2.40f},
        // "FIGHTING IS POINTLESS." (Stuck in flesh wall)
        {{"  ~~_..._~~~  ", " ~~(_____)~~  ", " ~~(_____)~~  ", "  ~~|||||~~~  ", "  ~~~~~~~~~~  "}, "FIGHTING IS POINTLESS.", 3.00f},
        {{"  ~~_..._~~~  ", " ~~(_____)~~  ", " ~~(_____)~~  ", "  ~~|||||~~~  ", "  ~~~~~~~~~~  "}, "FIGHTING IS POINTLESS.", 3.60f},
        {{"  ~~_..._~~~  ", " ~~(_____)~~  ", " ~~(_____)~~  ", "  ~~|||||~~~  ", "  ~~~~~~~~~~  "}, "FIGHTING IS POINTLESS.", 4.20f},
        {{"  ~~_..._~~~  ", " ~~(_____)~~  ", " ~~(_____)~~  ", "  ~~|||||~~~  ", "  ~~~~~~~~~~  "}, "FIGHTING IS POINTLESS.", 4.80f},
        {{"  ~~_..._~~~  ", " ~~(_____)~~  ", " ~~(_____)~~  ", "  ~~|||||~~~  ", "  ~~~~~~~~~~  "}, "FIGHTING IS POINTLESS.", 5.40f},
        // "THE MARBLE DOES NOT CARE." (Veins creeping)
        {{"  ##_..._###  ", " ##(#####)##  ", " ##(#|#|#)##  ", "  ##|||||###  ", "  ##########  "}, "THE MARBLE DOES NOT CARE.", 6.00f},
        {{"  ##_..._###  ", " ##(#####)##  ", " ##(#|#|#)##  ", "  ##|||||###  ", "  ##########  "}, "THE MARBLE DOES NOT CARE.", 6.60f},
        {{"  ##_..._###  ", " ##(#####)##  ", " ##(#|#|#)##  ", "  ##|||||###  ", "  ##########  "}, "THE MARBLE DOES NOT CARE.", 7.20f},
        {{"  ##_..._###  ", " ##(#####)##  ", " ##(#|#|#)##  ", "  ##|||||###  ", "  ##########  "}, "THE MARBLE DOES NOT CARE.", 7.80f},
        {{"  ##_..._###  ", " ##(#####)##  ", " ##(#|#|#)##  ", "  ##|||||###  ", "  ##########  "}, "THE MARBLE DOES NOT CARE.", 8.40f},
        // "IT EMBRACES YOU." (Swallowed completely)
        {{"  ##########  ", " ############ ", " ############ ", "  ##########  ", "  ##########  "}, "IT EMBRACES YOU.", 9.00f},
        {{"  ##########  ", " ############ ", " ############ ", "  ##########  ", "  ##########  "}, "IT EMBRACES YOU.", 9.60f},
        {{"  ##########  ", " ############ ", " ############ ", "  ##########  ", "  ##########  "}, "IT EMBRACES YOU.", 10.20f},
        {{"  ##########  ", " ############ ", " ############ ", "  ##########  ", "  ##########  "}, "IT EMBRACES YOU.", 10.80f},
        {{"  ##########  ", " ############ ", " ############ ", "  ##########  ", "  ##########  "}, "IT EMBRACES YOU.", 11.40f},
        // "SUBMIT." (Wall of trapped souls)
        {{"  ##########  ", " ##(OO)##()## ", " #()######()# ", " ##(OO)###### ", "  ##########  "}, "SUBMIT.", 12.00f},
        {{"  ##########  ", " ##(OO)##()## ", " #()######()# ", " ##(OO)###### ", "  ##########  "}, "SUBMIT.", 12.60f},
        {{"  ##########  ", " ##(OO)##()## ", " #()######()# ", " ##(OO)###### ", "  ##########  "}, "SUBMIT.", 13.20f},
        {{"  ##########  ", " ##(OO)##()## ", " #()######()# ", " ##(OO)###### ", "  ##########  "}, "SUBMIT.", 13.80f},
        {{"  ##########  ", " ##(OO)##()## ", " #()######()# ", " ##(OO)###### ", "  ##########  "}, "SUBMIT.", 14.40f}
    };

    // 
    // Choice 1: "I'M WAKING UP NOW"
    // Theme: Closed Eye -> Opening -> Black hole pupil -> Pulling faces -> Vault Door
    // 
    g_mindTrapMovies[4][1] = {
        // "THIS IS NO DREAM." (Closed Eye)
        {{"              ", "  __________  ", " (__________) ", "              ", "              "}, "THIS IS NO DREAM.", 0.00f},
        {{"              ", "  __________  ", " (__________) ", "              ", "              "}, "THIS IS NO DREAM.", 0.60f},
        {{"              ", "  __________  ", " (__________) ", "              ", "              "}, "THIS IS NO DREAM.", 1.20f},
        {{"              ", "  __________  ", " (__________) ", "              ", "              "}, "THIS IS NO DREAM.", 1.80f},
        {{"              ", "  __________  ", " (__________) ", "              ", "              "}, "THIS IS NO DREAM.", 2.40f},
        // "WAKE UP TO REALITY." (Eye Opening)
        {{"              ", "  .--------.  ", " (   (  )   ) ", "  '--------'  ", "              "}, "WAKE UP TO REALITY.", 3.00f},
        {{"              ", "  .--------.  ", " (   (  )   ) ", "  '--------'  ", "              "}, "WAKE UP TO REALITY.", 3.60f},
        {{"              ", "  .--------.  ", " (   (  )   ) ", "  '--------'  ", "              "}, "WAKE UP TO REALITY.", 4.20f},
        {{"              ", "  .--------.  ", " (   (  )   ) ", "  '--------'  ", "              "}, "WAKE UP TO REALITY.", 4.80f},
        {{"              ", "  .--------.  ", " (   (  )   ) ", "  '--------'  ", "              "}, "WAKE UP TO REALITY.", 5.40f},
        // "SEE WHAT YOU HAVE DONE." (Black Hole Pupil)
        {{"  .--------.  ", " (  (====)  ) ", " (  (====)  ) ", "  '--------'  ", "              "}, "SEE WHAT YOU HAVE DONE.", 6.00f},
        {{"  .--------.  ", " (  (====)  ) ", " (  (====)  ) ", "  '--------'  ", "              "}, "SEE WHAT YOU HAVE DONE.", 6.60f},
        {{"  .--------.  ", " (  (====)  ) ", " (  (====)  ) ", "  '--------'  ", "              "}, "SEE WHAT YOU HAVE DONE.", 7.20f},
        {{"  .--------.  ", " (  (====)  ) ", " (  (====)  ) ", "  '--------'  ", "              "}, "SEE WHAT YOU HAVE DONE.", 7.80f},
        {{"  .--------.  ", " (  (====)  ) ", " (  (====)  ) ", "  '--------'  ", "              "}, "SEE WHAT YOU HAVE DONE.", 8.40f},
        // "FEEL THE COLD SET IN." (Pulling Faces)
        {{"  .--------.  ", " ( (=(OO)=) ) ", " ( (=(OO)=) ) ", "  '--------'  ", "              "}, "FEEL THE COLD SET IN.", 9.00f},
        {{"  .--------.  ", " ( (=(OO)=) ) ", " ( (=(OO)=) ) ", "  '--------'  ", "              "}, "FEEL THE COLD SET IN.", 9.60f},
        {{"  .--------.  ", " ( (=(OO)=) ) ", " ( (=(OO)=) ) ", "  '--------'  ", "              "}, "FEEL THE COLD SET IN.", 10.20f},
        {{"  .--------.  ", " ( (=(OO)=) ) ", " ( (=(OO)=) ) ", "  '--------'  ", "              "}, "FEEL THE COLD SET IN.", 10.80f},
        {{"  .--------.  ", " ( (=(OO)=) ) ", " ( (=(OO)=) ) ", "  '--------'  ", "              "}, "FEEL THE COLD SET IN.", 11.40f},
        // "THERE IS NO ESCAPE." (Vault Door)
        {{"  [========]  ", "  [| o  o |]  ", "  [|  --  |]  ", "  [| ==== |]  ", "  [========]  "}, "THERE IS NO ESCAPE.", 12.00f},
        {{"  [========]  ", "  [| o  o |]  ", "  [|  --  |]  ", "  [| ==== |]  ", "  [========]  "}, "THERE IS NO ESCAPE.", 12.60f},
        {{"  [========]  ", "  [| o  o |]  ", "  [|  --  |]  ", "  [| ==== |]  ", "  [========]  "}, "THERE IS NO ESCAPE.", 13.20f},
        {{"  [========]  ", "  [| o  o |]  ", "  [|  --  |]  ", "  [| ==== |]  ", "  [========]  "}, "THERE IS NO ESCAPE.", 13.80f},
        {{"  [========]  ", "  [| o  o |]  ", "  [|  --  |]  ", "  [| ==== |]  ", "  [========]  "}, "THERE IS NO ESCAPE.", 14.40f}
    };

    // 
    // Choice 2: "I AM THE CREATOR!"
    // Theme: King on Throne -> Steps into vat -> Fluid Rises -> Submerged -> Exhaling Face (Loop back to start)
    // 
    g_mindTrapMovies[4][2] = {
        // "THE DIRECTOR STEPS DOWN." (King on Throne)
        {{"    \\ || /    ", "   -- () --   ", "   --/||\\--   ", "    / || \\    ", "   [======]   "}, "THE DIRECTOR STEPS DOWN.", 0.00f},
        {{"    \\ || /    ", "   -- () --   ", "   --/||\\--   ", "    / || \\    ", "   [======]   "}, "THE DIRECTOR STEPS DOWN.", 0.60f},
        {{"    \\ || /    ", "   -- () --   ", "   --/||\\--   ", "    / || \\    ", "   [======]   "}, "THE DIRECTOR STEPS DOWN.", 1.20f},
        {{"    \\ || /    ", "   -- () --   ", "   --/||\\--   ", "    / || \\    ", "   [======]   "}, "THE DIRECTOR STEPS DOWN.", 1.80f},
        {{"    \\ || /    ", "   -- () --   ", "   --/||\\--   ", "    / || \\    ", "   [======]   "}, "THE DIRECTOR STEPS DOWN.", 2.40f},
        // "HE TAKES HIS PLACE." (Steps into vat)
        {{"              ", "      ()      ", "     /||\\     ", "    |-||-|    ", "   [======]   "}, "HE TAKES HIS PLACE.", 3.00f},
        {{"              ", "      ()      ", "     /||\\     ", "    |-||-|    ", "   [======]   "}, "HE TAKES HIS PLACE.", 3.60f},
        {{"              ", "      ()      ", "     /||\\     ", "    |-||-|    ", "   [======]   "}, "HE TAKES HIS PLACE.", 4.20f},
        {{"              ", "      ()      ", "     /||\\     ", "    |-||-|    ", "   [======]   "}, "HE TAKES HIS PLACE.", 4.80f},
        {{"              ", "      ()      ", "     /||\\     ", "    |-||-|    ", "   [======]   "}, "HE TAKES HIS PLACE.", 5.40f},
        // "THE MASTERPIECE IS COMPLETE." (Fluid Rises)
        {{"              ", "    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   [======]   "}, "THE MASTERPIECE IS COMPLETE.", 6.00f},
        {{"              ", "    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   [======]   "}, "THE MASTERPIECE IS COMPLETE.", 6.60f},
        {{"              ", "    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   [======]   "}, "THE MASTERPIECE IS COMPLETE.", 7.20f},
        {{"              ", "    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   [======]   "}, "THE MASTERPIECE IS COMPLETE.", 7.80f},
        {{"              ", "    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   [======]   "}, "THE MASTERPIECE IS COMPLETE.", 8.40f},
        // "PERFECT PRESERVATION." (Submerged)
        {{"    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   |  ||  |   ", "   [======]   "}, "PERFECT PRESERVATION.", 9.00f},
        {{"    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   |  ||  |   ", "   [======]   "}, "PERFECT PRESERVATION.", 9.60f},
        {{"    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   |  ||  |   ", "   [======]   "}, "PERFECT PRESERVATION.", 10.20f},
        {{"    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   |  ||  |   ", "   [======]   "}, "PERFECT PRESERVATION.", 10.80f},
        {{"    ~~~~~~    ", "   |  ()  |   ", "   | /||\\ |   ", "   |  ||  |   ", "   [======]   "}, "PERFECT PRESERVATION.", 11.40f},
        // "ONE BREATH HELD FOREVER." (Exhaling Face - Loops to Phase 0)
        {{"    .----.    ", "   / o  o \\   ", "  |   /\\   |  ", "  |  .||.  |  ", "   \\ ---- /   "}, "ONE BREATH HELD FOREVER.", 12.00f},
        {{"    .----.    ", "   / o  o \\   ", "  |   /\\   |  ", "  |  .||.  |  ", "   \\ ---- /   "}, "ONE BREATH HELD FOREVER.", 12.60f},
        {{"    .----.    ", "   / o  o \\   ", "  |   /\\   |  ", "  |  .||.  |  ", "   \\ ---- /   "}, "ONE BREATH HELD FOREVER.", 13.20f},
        {{"    .----.    ", "   / o  o \\   ", "  |   /\\   |  ", "  |  .||.  |  ", "   \\ ---- /   "}, "ONE BREATH HELD FOREVER.", 13.80f},
        {{"    .----.    ", "   / o  o \\   ", "  |   /\\   |  ", "  |  .||.  |  ", "   \\ ---- /   "}, "ONE BREATH HELD FOREVER.", 14.40f}
    };


}

static void drawString11x16(Engine& engineContext, float x, float y, const std::string& text,
    SDL_Color color, int scale = 1)
{
    SDL_SetRenderDrawBlendMode(engineContext.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(engineContext.renderer, color.r, color.g, color.b, color.a);

    float currentX = x;

    for (char c : text) {
        if (c == '\n') {
            currentX = x;
            y += (16.0f * scale) + (2.0f * scale);
            continue;
        }

        // Calculate index (Font starts at ASCII 32 ' ')
        int asciiIndex = static_cast<int>(c) - 32;
        if (asciiIndex < 0 || asciiIndex >= 95) asciiIndex = 0;

        // Loop through the 16 horizontal rows
        for (int row = 0; row < 16; ++row) {
            uint16_t rowData = fontData11x16_RowMajor[asciiIndex][row];

            if (rowData == 0) continue; // Skip empty rows

            // Loop through the 11 pixels in this row
            for (int col = 0; col < 11; ++col) {
                // Read from left to right (bit 10 down to bit 0)
                if ((rowData >> (10 - col)) & 1) {
                    SDL_FRect pixelRect = {
                        currentX + (col * scale),
                        y + (row * scale),
                        (float)scale,
                        (float)scale
                    };
                    SDL_RenderFillRect(engineContext.renderer, &pixelRect);
                }
            }
        }

        // Advance to the next letter (11 pixels + 1 padding)
        currentX += (12.0f * scale);
    }
}

static void renderCaveQuiz( Engine &engineContext ) {
    if (!g_caveQuizActive || g_caveQuiz.empty()) return;
    if (g_caveQuizQuestionIndex < 0 || g_caveQuizQuestionIndex >= (int)g_caveQuiz.size()) return;

    const auto &q = g_caveQuiz[ g_caveQuizQuestionIndex ];

    int panelW = RENDER_W - 140;
    int panelH = 220;
    int x = (RENDER_W - panelW) / 2;
    int y = (RENDER_H - panelH) / 2;

    drawTextBox( engineContext, x, y, panelW, panelH, rgb( 12, 12, 16 ), rgb( 180, 150, 60 ) );
    std::string header = "Warden Statue " + std::to_string( g_caveQuizQuestionIndex + 1 ) + "/" + std::to_string( g_caveQuiz.size() );
    drawString16x16( engineContext, x + 16, y + 14, header, rgb( 255, 225, 120 ), panelW - 32, 1, 1, false );

    int cy = y + 46;
    cy = drawWrappedText( engineContext, x + 16, cy, q.question, rgb( 220, 220, 220 ), panelW - 32 );
    cy += 12;

    for (int i = 0; i < 4; ++i)
    {
        std::string line = std::to_string( i + 1 ) + ") " + q.options[ i ];
        drawString16x16( engineContext, x + 22, cy, line, rgb( 210, 210, 205 ), panelW - 44, 1, 1, false );
        cy += 24;
    }

    drawStringTinyScaled( engineContext, x + 16, y + panelH - 22, "Press 1-4 To Answer   ESC To Cancel", rgb( 130, 130, 145 ), 1, 1, 1, false );
}

static void renderMindTrapInterface(Engine& engineContext) {
    if (!g_mindTrapActive) return;
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;

    // Ensure movies are populated
    initMindTrapMovies();

    drawTextBox(engineContext, 0, 0, RENDER_W, RENDER_H, rgb(0, 0, 0), rgb(0, 0, 0));
    drawTranslucentBox(engineContext, 0, 0, RENDER_W, RENDER_H, rgb(4, 4, 6), 0.60f);

    const float petrification = (float)g_mindTrapPhaseIndex / std::max(1.0f, (float)g_mindTrapPhases.size());
    const Uint8 luma = Uint8(std::clamp(250.0f - (petrification * 120.0f), 110.0f, 250.0f));
    const Uint32 ink = rgb(luma, luma, luma);
    const Uint32 dimInk = rgb(std::max(45, int(luma) - 90), std::max(45, int(luma) - 90), std::max(55, int(luma) - 82));
    const Uint32 dangerInk = rgb(235, 85, 85);
    const Uint32 archiveInk = rgb(190, 210, 230);
    const Uint32 mindInk = rgb(245, 245, 245);
    const Uint32 subjectInk = rgb(150, 220, 165);
    const Uint32 noteInk = rgb(205, 175, 235);

    int panelX = 88;
    int panelY = 44;
    int panelW = RENDER_W - 176;
    int panelH = RENDER_H - 88;
    drawTranslucentBox(engineContext, panelX, panelY, panelW, panelH, rgb(0, 0, 0), 0.68f);

    const int charAdv = 6;
    const int rowStep = 12;
    const int cols = std::max(40, (panelW - 20) / charAdv);
    const int rows = std::max(20, (panelH - 20) / rowStep);
    const int tx = panelX + 10;
    const int ty = panelY + 8;

    const bool fractured = (std::sin(g_mindTrapFlickerTimer * 3.9f) > 0.84f);
    std::string topBorder = "+" + std::string(cols - 2, '-') + "+";
    std::string botBorder = topBorder;
    if (fractured && cols > 20)
    {
        topBorder.replace(cols / 2 - 4, 8, "/V\\/\\/");
    }

    drawStringTinyScaled(engineContext, tx, ty, topBorder, ink, 1, 1, 1, false);
    for (int r = 1; r < rows - 1; ++r)
    {
        drawStringTinyScaled(engineContext, tx, ty + r * rowStep, "|", dimInk, 1, 1, 1, false);
        drawStringTinyScaled(engineContext, tx + (cols - 1) * charAdv, ty + r * rowStep, "|", dimInk, 1, 1, 1, false);
    }
    drawStringTinyScaled(engineContext, tx, ty + (rows - 1) * rowStep, botBorder, ink, 1, 1, 1, false);

    drawStringTinyScaled(engineContext, tx + 12, ty + 14, "[SYSTEM_DIAGNOSTIC::NEURAL_LINK]", ink, 1, 1, 1, false);

    Uint32 dirInk = (int(g_mindTrapFlickerTimer * 4.0f) % 2 == 0) ? dangerInk : ink;
    drawStringTinyScaled(engineContext, tx + 12, ty + 26, "[USER_ID: DIRECTOR]  [STATUS: CORRUPTING]", dirInk, 1, 1, 1, false);

    drawStringTinyScaled(engineContext, tx + 12, ty + 38, "MEMORY", archiveInk, 1, 1, 1, false);
    drawStringTinyScaled(engineContext, tx + 62, ty + 38, "PSYCHE", mindInk, 1, 1, 1, false);
    drawStringTinyScaled(engineContext, tx + 112, ty + 38, "GUILT", subjectInk, 1, 1, 1, false);
    drawStringTinyScaled(engineContext, tx + 162, ty + 38, "NOTE", noteInk, 1, 1, 1, false);

    if (((int)(g_mindTrapFlickerTimer * 2.0f) % 5) == 1)
    {
        drawTranslucentBox(engineContext, tx + 12 * charAdv, ty + 26, 10 * charAdv, rowStep - 2, rgb(0, 0, 0), 0.96f);
    }

    auto cleanLine = [&](const std::string& raw) {
        std::string out = raw;
        if (out.rfind("VOICE> ", 0) == 0) out = "ARCHIVE> " + out.substr(7);
        else if (out.rfind("MIND> ", 0) == 0) out = "MIND> " + out.substr(6);
        else if (out.rfind("THOUGHT> ", 0) == 0) out = "NOTE> " + out.substr(9);
        else if (out.rfind("YOU> ", 0) == 0) out = "SUBJECT> " + out.substr(5);
        return normalizeMindTrapTerminalText(out);
        };

    const int logX = tx + 12;
    const int logY = ty + 48;
    const int logWChars = std::max(26, cols - 30);
    const int logRows = std::max(8, rows - 12);
    const int visibleLines = std::max(1, logRows);

    int start = 0;
    if ((int)g_mindTrapTerminalLog.size() > visibleLines) start = (int)g_mindTrapTerminalLog.size() - visibleLines;

    int ly = logY;
    for (int i = start; i < (int)g_mindTrapTerminalLog.size(); ++i)
    {
        std::string line = cleanLine(g_mindTrapTerminalLog[i]);
        if ((int)line.size() > logWChars) line = line.substr(0, logWChars - 3) + "...";
        Uint32 col = ink;
        Uint32 marker = dimInk;
        if (line.rfind("ARCHIVE>", 0) == 0) { col = archiveInk; marker = archiveInk; }
        else if (line.rfind("MIND>", 0) == 0) { col = mindInk; marker = mindInk; }
        else if (line.rfind("SUBJECT>", 0) == 0) { col = subjectInk; marker = subjectInk; }
        else if (line.rfind("NOTE>", 0) == 0) { col = noteInk; marker = noteInk; }
        if (line.find("CRITICAL_LOGIC_FAIL") != std::string::npos) col = dangerInk;

        drawTranslucentBox(engineContext, logX - 8, ly - 1, 4, rowStep - 2, marker, 0.95f);
        drawTranslucentBox(engineContext, logX - 2, ly - 1, (logWChars * charAdv) + 4, rowStep - 2, rgb(0, 0, 0), 0.35f);
        drawStringTinyScaled(engineContext, logX, ly, line, col, 1, 1, 1, false);

        ly += rowStep;
        if (ly > logY + (logRows - 1) * rowStep) break;
    }

    if (!g_mindTrapTypingLine.empty() && ly <= logY + (logRows - 1) * rowStep)
    {
        std::string typed = cleanLine(g_mindTrapTypingLine.substr(0, std::min(g_mindTrapTypingChars, g_mindTrapTypingLine.size())));
        if ((int)typed.size() > logWChars) typed = typed.substr(0, logWChars - 3) + "...";
        Uint32 typeCol = ink;
        if (typed.rfind("ARCHIVE>", 0) == 0) typeCol = archiveInk;
        else if (typed.rfind("MIND>", 0) == 0) typeCol = mindInk;
        else if (typed.rfind("SUBJECT>", 0) == 0) typeCol = subjectInk;
        else if (typed.rfind("NOTE>", 0) == 0) typeCol = noteInk;
        drawTranslucentBox(engineContext, logX - 2, ly - 1, (logWChars * charAdv) + 4, rowStep - 2, rgb(0, 0, 0), 0.35f);
        drawStringTinyScaled(engineContext, logX, ly, typed, typeCol, 1, 1, 1, false);
    }

    const MindTrapPhase& phase = g_mindTrapPhases[g_mindTrapPhaseIndex];
    const int galleryY = ty + (rows - 7) * rowStep;
    drawStringTinyScaled(engineContext, tx + 12, galleryY - 10, "[SELECTION_GALLERY::INTERPRETATIONS]", ink, 1, 1, 1, false);

    if (g_mindTrapAwaitingChoice)
    {
        for (int i = 0; i < 3; ++i)
        {
            const bool selected = (i == g_mindTrapSelectedOption);
            float sabotage = 0.0f;
            if (selected && isMindTrapDenialOption(phase.options[i]))
            {
                sabotage = std::clamp((g_mindTrapHoverTimer - 0.75f) / 1.65f, 0.0f, 1.0f);
            }

            std::string opt = phase.options[i];
            if (selected && sabotage > 0.01f) opt = mindTrapCorruptOptionText(opt, sabotage);

            int jitter = 0;
            if (selected && sabotage > 0.01f) jitter = int(std::round(std::sin(g_mindTrapChoiceJitterTimer * (18.0f + sabotage * 8.0f) + i * 0.7f) * std::min(1.0f, 0.35f + sabotage * 0.9f)));

            std::string line = std::to_string(i + 1) + ") " + opt;
            if ((int)line.size() > (cols - 18)) line = line.substr(0, cols - 21) + "...";
            const Uint32 col = selected ? (g_mindTrapForcedCorrectionActive ? dangerInk : ink) : dimInk;
            drawTranslucentBox(engineContext, tx + 12, galleryY + i * rowStep - 1, (cols - 22) * charAdv, rowStep - 2, rgb(0, 0, 0), selected ? 0.55f : 0.30f);
            drawStringTinyScaled(engineContext, tx + 14 + jitter, galleryY + i * rowStep, line, col, 1, 1, 1, false);
        }
    }

    if (g_mindTrapForcedCorrectionActive)
    {
        drawStringTinyScaled(engineContext, tx + 12, galleryY + 3 * rowStep + 2, "CRITICAL_LOGIC_FAIL: SUBJECT_MEMORY_CORRUPTED", dangerInk, 1, 1, 1, false);
    }

    if (!g_mindTrapShowingResult)
    {
        const std::vector<std::string> faceIdle = {
            "    .----.    ",
            "   /      \\   ",
            "  |  _  _  |  ",
            "  | | || | |  ",
            "  |  __    |  ",
            "   \\    /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceBlink = {
            "    .----.    ",
            "   /      \\   ",
            "  |        |  ",
            "  |  -- --  |  ",
            "  |  __    |  ",
            "   \\    /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceLookLeft = {
            "    .----.    ",
            "   /      \\   ",
            "  | _  _   |  ",
            "  || || |  |  ",
            "  |  __    |  ",
            "   \\    /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceLookRight = {
            "    .----.    ",
            "   /      \\   ",
            "  |   _  _ |  ",
            "  |  | || ||  ",
            "  |  __    |  ",
            "   \\    /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceLookUp = {
            "    .----.    ",
            "   / _  _ \\   ",
            "  | | || | |  ",
            "  |        |  ",
            "  |  __    |  ",
            "   \\    /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceLookDown = {
            "    .----.    ",
            "   /      \\   ",
            "  |        |  ",
            "  |  _  _  |  ",
            "  | | || | |  ",
            "   \\ __ /   ",
            "   /____\\   "
        };

        const std::vector<std::string> faceScary = {
            "    .----.    ",
            "   /      \\   ",
            "  |  O  O  |  ",
            "  |  |  |  |  ",
            "  | /VVVV\\ |  ",
            "   \\      /   ",
            "   /____\\   "
        };

        static int frameCounter = 0;
        frameCounter++;

        const std::vector<std::string>* currentFace = &faceIdle;

        int lookCycle = (frameCounter / 120) % 10;

        bool isBlinking = (frameCounter % 180) < 6;

        bool isScary = (frameCounter % 1500) < 4;

        if (isScary)
        {
            currentFace = &faceScary; // Highest priority: overrides everything
        }
        else if (isBlinking)
        {
            currentFace = &faceBlink; // Blinking overrides looking around
        }
        else
        {
            switch (lookCycle) {
            case 2: currentFace = &faceLookLeft;  break;
            case 5: currentFace = &faceLookRight; break;
            case 7: currentFace = &faceLookUp;    break;
            case 9: currentFace = &faceLookDown;  break;
            default: currentFace = &faceIdle;     break; // Idle on 0, 1, 3, 4, 6, 8
            }
        }

        int artX = tx + (cols - 28) * charAdv;
        int artY = ty + 54;

        for (int i = 0; i < (int)currentFace->size(); ++i) {
            drawStringTinyScaled(engineContext, artX, artY + i * rowStep, (*currentFace)[i], dimInk, 1, 1, 1, false);
        }
    }

    if (g_mindTrapShowingResult)
    {
        float elapsedMovieTime = 15.0f - g_mindTrapResultTimer;

        const int safePhase = std::clamp(g_mindTrapPhaseIndex, 0, 4);
        const int safeChoice = std::clamp(g_mindTrapSelectedOption, 0, 2);

        if (g_mindTrapMovies.size() > safePhase && g_mindTrapMovies[safePhase].size() > safeChoice && !g_mindTrapMovies[safePhase][safeChoice].empty())
        {
            const auto& movieFrames = g_mindTrapMovies[safePhase][safeChoice];

            const MindTrapMovieFrame* currentFrame = &movieFrames[0];
            for (const auto& frame : movieFrames) {
                if (elapsedMovieTime >= frame.timestamp) {
                    currentFrame = &frame;
                }
            }

            int mw = 420;
            int mh = 260;
            int mx = panelX + (panelW - mw) / 2;
            int my = panelY + (panelH - mh) / 2;

            drawTranslucentBox(engineContext, mx, my, mw, mh, rgb(8, 8, 12), 0.95f);

            Uint32 borderCol = rgb(150, 40, 40);
            for (int x = mx; x < mx + mw; ++x) { putPix(engineContext, x, my, borderCol); putPix(engineContext, x, my + mh - 1, borderCol); }
            for (int y = my; y < my + mh; ++y) { putPix(engineContext, mx, y, borderCol); putPix(engineContext, mx + mw - 1, y, borderCol); }

            drawStringTinyScaled(engineContext, mx + 10, my + 10, "[ VISUAL_RESPONSE_PLAYBACK ]", borderCol, 1, 1, 1, false);

            int artScale = 2;
            int artCharW = 6 * artScale;
            int artLineH = 12 * artScale;
            int asciiW = (int)currentFrame->lines[0].size() * artCharW;
            int asciiH = (int)currentFrame->lines.size() * artLineH;
            int asciiX = mx + (mw - asciiW) / 2;
            int asciiY = my + (mh - asciiH) / 2 - 20;

            for (int i = 0; i < (int)currentFrame->lines.size(); ++i) {
                drawStringTinyScaled(engineContext, asciiX + 20, asciiY + (i * artLineH), currentFrame->lines[i], ink, artScale, 1, 1, false);
            }

            int subW = (int)currentFrame->subtitle.size() * charAdv;
            int subX = mx + (mw - subW) / 2;
            int subY = my + mh - 35;
            drawStringTinyScaled(engineContext, subX, subY, currentFrame->subtitle, mindInk, 2, 1, 1, false);
        }
    }

    if (g_mindTrapTearActive)
    {
        const float tearP = std::clamp(g_mindTrapTearTimer / 1.5f, 0.0f, 1.0f);
        const int tearRows = int(RENDER_H * tearP);
        const std::string glyphs = "#/\\|_-=+*[]{}";
        for (int y = 0; y < tearRows; y += 12)
        {
            std::string row;
            row.reserve(cols);
            for (int c = 0; c < cols; ++c)
            {
                int pick = int(std::fabs(std::sin((float)c * 0.67f + (float)y * 0.11f + g_mindTrapTearTimer * 12.0f)) * (glyphs.size() - 1));
                row.push_back(glyphs[std::clamp(pick, 0, (int)glyphs.size() - 1)]);
            }
            drawStringTinyScaled(engineContext, tx, y, row, rgb(190, 190, 190), 1, 1, 1, false);
        }

        if (tearP > 0.45f)
        {
            drawTextBox(engineContext, 0, (RENDER_H / 2) - 22, RENDER_W, 44, rgb(0, 0, 0), rgb(0, 0, 0));
            drawString16x16(engineContext, (RENDER_W / 2) - 180, (RENDER_H / 2) - 4, "IT WAS YOUR HAND.", rgb(245, 245, 245), 360, 1, 1, false);
        }
    }

    if (g_mindTrapWhiteFlashTimer > 0.0f)
    {
        const float flash = std::clamp(g_mindTrapWhiteFlashTimer / 0.65f, 0.0f, 1.0f);
        const float alpha = std::clamp(std::pow(flash, 0.65f), 0.0f, 1.0f);
        drawTranslucentBox(engineContext, 0, 0, RENDER_W, RENDER_H, rgb(255, 255, 255), alpha);
    }
}

static void renderInteractionAnimation( Engine &engineContext ) {
    if (!g_interactionAnim.active) return;

    float p = std::clamp( g_interactionAnim.t / std::max( 0.001f, g_interactionAnim.duration ), 0.0f, 1.0f );
    int w = 520;
    int h = 126;
    int camShiftX = int( std::sin( p * 3.14159265f ) * 14.0f );
    int x = (RENDER_W - w) / 2 + camShiftX;
    int y = RENDER_H - h - 28;

    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), 0.18f );
    drawTextBox( engineContext, x, y, w, h, rgb( 8, 8, 12 ), rgb( 165, 138, 70 ) );
    drawString16x16( engineContext, x + 16, y + 12, g_interactionAnim.label, rgb( 235, 220, 170 ), w - 32, 1, 1, false );

    int stageY = y + 54;
    int stageH = 52;
    drawTextBox( engineContext, x + 14, stageY, w - 28, stageH, rgb( 12, 12, 16 ), rgb( 80, 80, 95 ) );

    if (g_interactionAnim.type == InteractionAnimType::KEY_USE)
    {
        int lockX = x + w - 118;
        int lockY = stageY + 12;
        drawTextBox( engineContext, lockX, lockY, 56, 28, rgb( 50, 44, 34 ), rgb( 170, 148, 96 ) );
        drawTextBox( engineContext, lockX + 18, lockY + 7, 20, 14, rgb( 18, 18, 18 ), rgb( 130, 110, 70 ) );

        float keyMotion = std::clamp( p * 0.85f, 0.0f, 1.0f );
        int keyStartX = x + 34;
        int keyTargetX = lockX + 10;
        int keyX = keyStartX + int( (keyTargetX - keyStartX) * keyMotion );
        int keyY = stageY + 26;

        Uint32 keyCol = rgb( 210, 176, 88 );
        for (int yy = -5; yy <= 5; ++yy)
        {
            for (int xx = -5; xx <= 5; ++xx)
            {
                if (xx * xx + yy * yy <= 25) putPix( engineContext, keyX + xx, keyY + yy, keyCol );
            }
        }
        for (int xx = 6; xx <= 30; ++xx) putPix( engineContext, keyX + xx, keyY, keyCol );
        putPix( engineContext, keyX + 26, keyY + 1, keyCol );
        putPix( engineContext, keyX + 27, keyY + 1, keyCol );
        putPix( engineContext, keyX + 26, keyY + 2, keyCol );

        if (p > 0.72f)
        {
            drawStringTinyScaled( engineContext, x + w - 176, y + h - 16, "Turning...", rgb( 220, 190, 120 ), 1, 1, 1, false );
        }
    }
    else if (g_interactionAnim.type == InteractionAnimType::NOTE_COLLECT)
    {
        int nx = x + 40;
        int ny = stageY + 6;
        drawTextBox( engineContext, nx, ny, 72, 40, rgb( 210, 198, 164 ), rgb( 120, 96, 64 ) );
        drawStringTinyScaled( engineContext, nx + 10, ny + 12, "Note", rgb( 70, 55, 36 ), 1, 1, 1, false );
    }
    else if (g_interactionAnim.type == InteractionAnimType::ITEM_PICKUP)
    {
        int cx = x + 70;
        int cy = stageY + 26;
        for (int yy = -8; yy <= 8; ++yy)
        {
            for (int xx = -8; xx <= 8; ++xx)
            {
                if (xx * xx + yy * yy <= 64) putPix( engineContext, cx + xx, cy + yy, rgb( 160, 200, 120 ) );
            }
        }
        drawStringTinyScaled( engineContext, x + 110, stageY + 22, "ACQUIRED", rgb( 180, 220, 145 ), 1, 1, 1, false );
    }
    else
    {
        int barX = x + 24;
        int barY = stageY + 18;
        int barW = w - 48;
        int barH = 16;
        drawTextBox( engineContext, barX, barY, barW, barH, rgb( 12, 12, 12 ), rgb( 90, 90, 105 ) );
        int fill = (int)((barW - 2) * p);
        for (int yy = barY + 1; yy < barY + barH - 1; ++yy)
        {
            for (int xx = barX + 1; xx < barX + 1 + fill; ++xx)
            {
                putPix( engineContext, xx, yy, rgb( 195, 165, 85 ) );
            }
        }
    }
}

static void renderLevelTransitionOverlay( Engine &engineContext ) {
    if (!g_levelTransition.active) return;

    float p = std::clamp( g_levelTransition.t / std::max( 0.001f, g_levelTransition.duration ), 0.0f, 1.0f );
    float fade = (p < 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.95f - fade * 0.65f, 0.2f, 0.95f ) );
}

static void buildSimpleKeySprite( Image &img, Uint32 keyColor ) {
    img.width = 24;
    img.height = 24;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height)
        {
            img.pixels[ y * img.width + x ] = c;
        }
        };

    for (int y = 6; y <= 14; ++y)
    {
        for (int x = 4; x <= 12; ++x)
        {
            int dx = x - 8;
            int dy = y - 10;
            if (dx * dx + dy * dy <= 14) p( x, y, keyColor );
        }
    }
    for (int x = 12; x <= 20; ++x) p( x, 10, keyColor );
    for (int x = 16; x <= 18; ++x)
    {
        p( x, 11, keyColor );
        p( x, 12, keyColor );
    }
    p( 19, 9, keyColor );
    p( 20, 9, keyColor );
}

static int addKeyPickupSprite( Engine &engineContext, float x, float y, const std::string &keyName, Uint32 keyColor ) {
    Image img;
    buildSimpleKeySprite( img, keyColor );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "PUZZLE_KEY";
    prop.filename = keyName;
    prop.textureID = texId;
    prop.scale = 0.45f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static int addNotePickupSprite( Engine &engineContext, float x, float y, const std::string &noteName );

static NotePickupVisual addNotePickupModel( Engine &engineContext, float x, float y, const std::string &noteName ) {
    NotePickupVisual out;
    out.propIndex = addNotePickupSprite( engineContext, x, y, noteName );
    if (out.propIndex >= 0 && out.propIndex < (int)engineContext.props.size())
    {
        engineContext.props[ out.propIndex ].scale = 0.0f;
    }

    out.modelIndex = addWorldModelInstance(
        resolveAssetModelPath( "Note.glb" ),
        x,
        y,
        0.25f,
        rgb( 225, 214, 180 ),
        0.0f,
        0.0f,
        0.0f,
        true,
        1.4f );

    return out;
}
static ClueNote makeClueNote(Engine& engineContext, const std::string& title, const std::string& body, float x, float y, Levels level = Levels::MUSEUM, bool spawnVisuals = true) {
    ClueNote note;
    note.title = title;
    note.body = body;
    note.x = x;
    note.y = y;
    note.collected = false;
    note.level = level;

    if (!spawnVisuals || level != engineContext.currentLevel)
    {
        note.propIndex = -1;
        note.modelIndex = -1;
        return note;
    }

    NotePickupVisual vis = addNotePickupModel(engineContext, x, y, title);
    note.propIndex = vis.propIndex;
    note.modelIndex = vis.modelIndex;
    return note;
}

static KeyPickup addKeyPickupModelProxy(Engine& engineContext, const std::string& keyName, float x, float y, Uint32 keyColor, const std::string& modelAsset, Levels level = Levels::MUSEUM, float modelHeightOffset = 0.2f, bool spawnVisuals = true) {
    KeyPickup out;
    out.keyName = keyName;
    out.x = x;
    out.y = y;
    out.collected = false;
    out.level = level;
    out.modelHeightOffset = modelHeightOffset;

    if (!spawnVisuals || level != engineContext.currentLevel)
    {
        out.propIndex = -1;
        out.modelIndex = -1;
        return out;
    }

    float roll = -1.5707963f;
    float scale = 0.17f;
    bool spin = true;

    if (keyName == "BLACK PIGMENT" || keyName == "BLUE PIGMENT" || keyName == "RED PIGMENT")
    {
        roll = 0.0f;
        scale = 0.14f;
        spin = false;
    }

    int spriteIndex = addKeyPickupSprite(engineContext, x, y, keyName, keyColor);
    int modelIndex = addWorldModelInstance(resolveAssetModelPath(modelAsset), x, y, scale, keyColor, 0.0f, 0.0f, roll, spin, 1.2f, modelHeightOffset);

    if (spriteIndex >= 0 && spriteIndex < (int)engineContext.props.size() && modelIndex >= 0)
    {
        engineContext.props[spriteIndex].scale = 0.0f;
    }

    out.propIndex = spriteIndex;
    out.modelIndex = modelIndex;
    return out;
}

static void buildStairWallOverlay( Image &img ) {
    img.width = 96;
    img.height = 96;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    Uint32 wallShadow = rgb( 38, 42, 52 );
    Uint32 wallEdge = rgb( 78, 84, 98 );
    Uint32 stepTop = rgb( 142, 148, 158 );
    Uint32 stepFront = rgb( 92, 98, 110 );

    for (int y = 12; y <= 92; ++y)
    {
        for (int x = 14; x <= 82; ++x)
        {
            p( x, y, wallShadow );
        }
    }
    for (int x = 14; x <= 82; ++x) { p( x, 12, wallEdge ); p( x, 92, wallEdge ); }
    for (int y = 12; y <= 92; ++y) { p( 14, y, wallEdge ); p( 82, y, wallEdge ); }

    int left = 24;
    int right = 72;
    int y = 86;
    for (int s = 0; s < 8; ++s)
    {
        int topY = y - 3;
        for (int xx = left; xx <= right; ++xx) p( xx, topY, stepTop );
        for (int yy = topY + 1; yy <= y; ++yy)
            for (int xx = left; xx <= right; ++xx)
                p( xx, yy, stepFront );

        left += 2;
        right -= 2;
        y -= 9;
    }
}

static void buildSimpleNoteSprite( Image &img ) {
    img.width = 24;
    img.height = 24;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 paper = rgb( 225, 214, 180 );
    Uint32 ink = rgb( 60, 50, 35 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height)
        {
            img.pixels[ y * img.width + x ] = c;
        }
        };

    for (int y = 4; y <= 19; ++y)
    {
        for (int x = 5; x <= 18; ++x)
        {
            p( x, y, paper );
        }
    }
    for (int x = 7; x <= 16; ++x)
    {
        p( x, 8, ink );
        p( x, 11, ink );
        p( x, 14, ink );
    }
}

static int addNotePickupSprite( Engine &engineContext, float x, float y, const std::string &noteName ) {
    Image img;
    buildSimpleNoteSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "CLUE_NOTE";
    prop.filename = noteName;
    prop.textureID = texId;
    prop.scale = 0.38f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static void buildSafeSprite( Image &img ) {
    img.width = 48;
    img.height = 40;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 frontMid = rgb( 108, 116, 124 );
    Uint32 frontDark = rgb( 74, 80, 88 );
    Uint32 topCol = rgb( 142, 150, 160 );
    Uint32 sideCol = rgb( 84, 92, 102 );
    Uint32 rim = rgb( 52, 56, 62 );
    Uint32 dial = rgb( 188, 198, 210 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    // top face (faux 3D)
    for (int y = 4; y <= 10; ++y)
    {
        for (int x = 10; x <= 37; ++x)
        {
            p( x, y, topCol );
        }
    }

    // front face
    for (int y = 11; y <= 34; ++y)
    {
        for (int x = 8; x <= 37; ++x)
        {
            Uint32 c = (x < 15) ? frontDark : frontMid;
            p( x, y, c );
        }
    }

    // right side face
    for (int y = 11; y <= 34; ++y)
    {
        for (int x = 38; x <= 43; ++x)
        {
            p( x, y, sideCol );
        }
    }

    // rims
    for (int x = 8; x <= 43; ++x) { p( x, 11, rim ); p( x, 34, rim ); }
    for (int y = 11; y <= 34; ++y) { p( 8, y, rim ); p( 43, y, rim ); }

    // dial + handle
    for (int y = 19; y <= 25; ++y)
    {
        for (int x = 20; x <= 26; ++x)
        {
            p( x, y, dial );
        }
    }
    for (int x = 27; x <= 31; ++x) p( x, 22, dial );
}

static int addSafeSprite( Engine &engineContext, float x, float y, const std::string &name ) {
    Image img;
    buildSafeSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "SAFE";
    prop.filename = name;
    prop.textureID = texId;
    prop.scale = 0.95f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static void buildPedestalSprite( Image &img ) {
    img.width = 48;
    img.height = 56;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 light = rgb( 170, 162, 145 );
    Uint32 mid = rgb( 146, 136, 118 );
    Uint32 dark = rgb( 112, 102, 88 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    // top slab
    for (int y = 4; y <= 11; ++y)
        for (int x = 10; x <= 37; ++x)
            p( x, y, light );

    // shaft front
    for (int y = 12; y <= 45; ++y)
        for (int x = 14; x <= 33; ++x)
            p( x, y, (x < 22) ? mid : light );

    // shaft side
    for (int y = 12; y <= 45; ++y)
        for (int x = 34; x <= 39; ++x)
            p( x, y, dark );

    // base plinth
    for (int y = 46; y <= 53; ++y)
    {
        for (int x = 8; x <= 39; ++x)
        {
            p( x, y, (x < 24) ? dark : mid );
        }
    }
}

static int addPedestalSprite( Engine &engineContext, float x, float y, const std::string &name ) {
    Image img;
    buildPedestalSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "PEDESTAL";
    prop.filename = name;
    prop.textureID = texId;
    prop.scale = 1.05f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static BoxProp buildStairStepBox( float centerX, float centerY, float halfLength, float halfDepth, float height, float angle, const Image &tex ) {
    BoxProp step;
    step.centerX = centerX;
    step.centerY = centerY;
    step.halfLength = halfLength;
    step.halfDepth = halfDepth;
    step.height = height;
    step.angle = angle;
    step.sideTexure = tex;
    step.legTexure = tex;
    step.legHalf = 0.03f;
    step.legInsetLength = 0.02f;
    step.legInsetDepth = 0.02f;
    return step;
}

static Image makeSafeMetalTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 140, 150, 162 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int n = ((x * 17 + y * 31) & 7) - 3;
            int shade = std::clamp( 150 + n - (x / 5), 132, 212 );
            tex.pixels[ y * tex.width + x ] = rgb( shade, shade + 6, shade + 14 );
        }
    }
    return tex;
}

static Image makeSafeDoorTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 136, 146, 156 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int base = 132 + ((x + y) & 7);
            tex.pixels[ y * tex.width + x ] = rgb( base, base + 8, base + 14 );
        }
    }
    for (int y = 18; y <= 46; ++y)
    {
        for (int x = 18; x <= 46; ++x)
        {
            int dx = x - 32;
            int dy = y - 32;
            int r2 = dx * dx + dy * dy;
            if (r2 <= 100)
            {
                tex.pixels[ y * tex.width + x ] = rgb( 176, 188, 205 );
            }
            else if (r2 <= 144)
            {
                tex.pixels[ y * tex.width + x ] = rgb( 128, 138, 152 );
            }
        }
    }
    return tex;
}

static Image makeStoneTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 172, 162, 144 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int n = ((x * 13 + y * 23 + (x * y) / 9) & 15) - 7;
            int shade = std::clamp( 168 + n, 138, 214 );
            tex.pixels[ y * tex.width + x ] = rgb( shade, shade - 8, shade - 20 );
        }
    }
    return tex;
}

static int addBoxMesh( Engine &engineContext, float x, float y, float halfLength, float halfDepth, float height, const Image &tex ) {
    BoxProp box;
    box.centerX = x;
    box.centerY = y;
    box.halfLength = halfLength;
    box.halfDepth = halfDepth;
    box.height = height;
    box.angle = 0.0f;
    box.sideTexure = tex;
    box.legTexure = tex;
    box.legHalf = 0.0f;
    box.legInsetLength = 0.0f;
    box.legInsetDepth = 0.0f;
    int index = (int)engineContext.benches3D.size();
    engineContext.benches3D.push_back( std::move( box ) );
    return index;
}

static void addSafe3D( Engine &engineContext, float x, float y ) {
    (void)engineContext;
    g_safeBoxIndices.push_back( addWorldModelInstance( resolveAssetModelPath( "Safe.glb" ), x, y, 0.72f, rgb( 255, 255, 255 ), 3.14, 0, 0, false, 0, -0.05f) );
}

static void addPedestal3D( Engine &engineContext, float x, float y ) {
    (void)engineContext;
    g_pedestalBoxIndices.push_back( addWorldModelInstance( resolveAssetModelPath( "Pedestal.glb" ), x, y, 0.46f, rgb( 164, 156, 142 ), 0, 0, false, 0, -0.05f) );
}

static void initMuseumPuzzle(Engine& engineContext) {
    g_roomLocks = {
        // Doors blocking the main 4 wings:
        {6, 9, "West Wing", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM},
        {10, 6, "North Wing", LockType::CODE, "0300", false, Levels::MUSEUM},
        {16, 9, "East Wing", LockType::KEY, "GOLD KEY", false, Levels::MUSEUM},
        {10, 12, "South Wing", LockType::CODE, "7391", false, Levels::MUSEUM},
        // Doors blocking the new 4 corner rooms:
        {5, 2, "NW Archives", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM}, // From NW
        {14, 3, "NE Vault", LockType::KEY, "IRON KEY", false, Levels::MUSEUM}, // From NW
        {5, 15, "SW Crypt", LockType::KEY, "SILVER KEY", false, Levels::MUSEUM}, // From SW
        {17, 13, "SE Office", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM}, // From East
        // Second-floor puzzle gates
        {6, 9, "Infinite Archive", LockType::CODE, "1911", true, Levels::MUSEUM_UPPER},
        {10, 6, "Taxidermy Studio", LockType::CODE, "1490", false, Levels::MUSEUM_UPPER}
    };

    g_playerKeys.clear();
    g_foundNotes.clear();
    g_accessPopup.clear();
    g_accessPopupUntil = 0;
    g_codeEntryActive = false;
    g_codeEntryLockIndex = -1;
    g_safeEntryIndex = -1;
    g_symbolEntryIndex = -1;
    g_codeEntryBuffer.clear();
    g_notesOpen = false;
    g_caveFinalNoteCollected = false;
    g_caveTimerActive = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    g_caveQuiz.clear();
    g_mindTrapActive = false;
    g_mindTrapTriggerConsumed = false;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_mindTrapLastHoveredOption = -1;
    g_mindTrapHoverTimer = 0.0f;
    g_mindTrapForcedCorrectionActive = false;
    g_mindTrapForcedCorrectionTimer = 0.0f;
    g_mindTrapForcedOption = -1;
    g_mindTrapChoiceJitterTimer = 0.0f;
    g_mindTrapTearActive = false;
    g_mindTrapTearTimer = 0.0f;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_mindTrapLastHoveredOption = -1;
    g_mindTrapHoverTimer = 0.0f;
    g_mindTrapForcedCorrectionActive = false;
    g_mindTrapForcedCorrectionTimer = 0.0f;
    g_mindTrapForcedOption = -1;
    g_mindTrapChoiceJitterTimer = 0.0f;
    g_mindTrapTearActive = false;
    g_mindTrapTearTimer = 0.0f;
    g_solventLabUnlockCutsceneActive = false;
    g_solventLabUnlockCutsceneStage = 0;
    g_solventLabUnlockCutsceneTimer = 0.0f;
    g_solventLabUnlockTurnStartYaw = 0.0f;
    g_solventLabUnlockWhiteFlash = 0.0f;
    if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
    {
        g_worldModels[ g_solventLabMonsterModelIndex ].visible = false;
    }
    g_solventLabMonsterModelIndex = -1;
    g_solventCoolerEntryActive = false;
    g_solventCoolerBuffer.clear();
    g_redPigmentDispensed = false;
    g_redPigmentDispenseCutsceneActive = false;
    g_redPigmentDispenseCutsceneTimer = 0.0f;
    g_redPigmentDispenseModelIndex = -1;
    g_redPigmentDispenseBaseYaw = 0.0f;
    g_restorationWingUnlocked = false;
    g_directorDeskUnlocked = false;
    g_combatState = {};
    g_showHeldWeapon = true;
    g_revolverPickup = {};
    g_heldRevolverModelIndex = -1;
    g_revolverAiming = false;
    g_revolverShotCooldown = 0.0f;
    g_revolverRecoilTimer = 0.0f;
    g_revolverInspectCutsceneActive = false;
    g_revolverInspectCutsceneTimer = 0.0f;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    resetWhisperAmbience();
    g_firstLockedDoorDialogueShown = false;
    g_generatorNeedsGasLinePlayed = false;
    g_gasCanCollected = false;
    g_generatorFueled = false;
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;
    g_powerRestoreFlickerActive = false;
    g_powerRestoreFlickerTimer = 0.0f;
    g_cutsceneController.reset();
    g_dialogue.clear();
    g_safes.clear();
    g_safeBoxIndices.clear();
    g_symbols.clear();
    g_pedestalBoxIndices.clear();
    g_clueNotes.clear();

    if (!g_stairWallOverlayReady)
    {
        buildStairWallOverlay( g_stairWallOverlay );
        g_stairWallOverlayReady = true;
    }

    g_keyPickups.clear();

    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "BLUE PIGMENT", 9.9f, 2.9f, rgb( 90, 140, 220 ), "BluePigment.glb", Levels::MUSEUM_UPPER, 0.25f ) );
    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "BLACK PIGMENT", 9.25f, 13.85f, rgb( 90, 140, 220 ), "BlackPigment.glb", Levels::MUSEUM_UPPER, 0.30f ) );

    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "BRONZE KEY", 15.f, 8.f, rgb( 180, 120, 40 ), "Bronze Key.glb" ) );
    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "SILVER KEY", 10.5f, 3.5f, rgb( 190, 190, 200 ), "Silver Key.glb" ) );
    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "DIRECTOR'S KEY", 4.5f, 15.5f, rgb( 210, 170, 95 ), "Bronze Key.glb" ) );

    g_safes.clear();
    g_safeBoxIndices.clear();
    g_safes.push_back( { "Director's Safe", "2026", 18.7f, 16.7f, false, "GOLD KEY" } );
    addSafe3D( engineContext, 18.7f, 16.7f );

    g_symbols.clear();
    g_pedestalBoxIndices.clear();
    g_symbols.push_back( { "Ancient Pedestal", { 0, 2, 1 }, 3.5f, 3.5f, false, "IRON KEY" } );
    addPedestal3D( engineContext, 3.5f, 3.5f );

    g_revolverPickup.weaponName = "REVOLVER";
    g_revolverPickup.x = 17.95f;
    g_revolverPickup.y = 16.1f;
    g_revolverPickup.level = Levels::MUSEUM;
    g_revolverPickup.collected = true;
    g_revolverPickup.modelIndex = -1;

    g_clueNotes.clear();
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Missed Calls",
        "12 missed calls. No signal. No contacts. Why is my battery dropping so fast?",
        10.0f, 10.3f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Archivist Notebook",
        "The new centerpiece requires strict alignment before the locking mechanism engages. The Director was very specific about the symbolism. First, the vehicle of soldiers many centuries ago. Second, the cult of the woods, the hunted. Finally, the apex-predator of the wild forest. Align them in this order, or the vault remains sealed.",
        3.5f, 8.5f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Security Log",
        "The North Wing lockdown code is the year of the four rulers. Do not forget it.",
        4.5f, 10.5f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Director Memo",
        "To the Board of Curators: We are on the verge of a breakthrough in absolute preservation. The flesh decays, but our new solvent arrests time entirely. To commemorate this milestone, my personal safe has been secured with the current calendar year. Inside is the key to my desk, and by extension, the future of our collection. Do not disturb me; I am preparing the newest acquisition.",
        3.5f, 15.5f ) );

    g_clueNotes.push_back( makeClueNote( engineContext,
        "Conservation Log A",
        "We preserve the beauty of the frozen moment. Time should stop before decay can argue.",
        7.8f, 6.8f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Conservation Log B",
        "A perfect exhibit is one breath held forever. Preservation is mercy, not violence.",
        14.7f, 6.2f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Conservation Log C",
        "Stillness is purity. If they move, they suffer. If they freeze, they become art.",
        11.4f, 13.8f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Janitor Note",
        "Maintenance Request #44: I'm begging you to fix the keypad on the South Wing doors. The buttons are sticking again. If there's an emergency, I don't want to be fumbling to type 7-3-9-1 while the lockdown sirens are screaming. Also, please tell the night staff to stop moving the exhibits. I swear the stag was facing the other way yesterday.",
        17.5f, 2.5f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Spilled Solvent",
        "The red stains won't come up with standard bleach. The Director says it's 'Special Oil.' It smells like a hospital. I don't understand what the director wants. We take the consciousness out of people like a modern lobotomy. We have failed to capture the mind of any subject. I hope rumors of the escapee are false, we've already lost three. Maybe the subject is looking for the humanity we took. As the director says: One breath held forever",
        5.5f, 8.7f,
        Levels::MUSEUM_UPPER ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Entry #402",
        "I can still see the fear in his eyes. The color drained from his body. The texture abandoned his face. He went limp. The Director's requests are becoming too much.",
        11.1f, 4.5f,
        Levels::MUSEUM_UPPER ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Scientist Note",
        "It's alive. I don't know how it happened. The doors just locked. It's only a matter of time now... The taxidermy studio code is held within the triptych of both human pleasure and torture",
        9.8f, 15.6f,
        Levels::MUSEUM_UPPER ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Survivor Note",
        "Look away. If you look it in the eye it will take you. We managed to lock it in the bioroom",
        14.2f, 9.5f,
        Levels::MUSEUM_UPPER ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Special Exhibit Intake Receipt",
        "ACQUISITION LOG 804\nCondition: Subject is conscious, highly disoriented, exhibiting elevated heart rate.\nTreatment: Solvent applied. Paralysis setting in at standard rate.\nCurator Notes: The subject believes they are exploring the facility. Let them wander. The exertion accelerates the calcification process. By the time they reach the lower levels, their legs will turn to marble. We will display them in the West Wing.",
        9.8f, 5.0f,
        Levels::MUSEUM_UPPER ) );

    g_museumPuzzleInitialized = true;
}
static void initCaveQuiz() {
    g_caveQuiz.clear();

    // Q1 focuses on the Director's twisted philosophy (Conservation Log B)
    g_caveQuiz.push_back({
        "According to the Director, what makes the perfect exhibit?",
        {"One breath held forever", "A flawless recreation of history", "Abstract interpretation", "Pure, untouched marble"},
        0
        });

    // Q2 focuses on the conversion process (Conservation Log C)
    g_caveQuiz.push_back({
        "At what exact moment does the subject finally become art?",
        {"When the frame is sealed", "When the solvent is applied", "When they freeze", "When the public arrives"},
        2
        });

    // Q3 focuses on the player's own grim realization (Intake Receipt)
    g_caveQuiz.push_back({
        "What is the physical condition of the newest acquisition?",
        {"Mummified in ash", "Conscious and disoriented", "Preserved in formaldehyde", "Asleep and unfeeling"},
        1
        });
}

static void initCaveFinalObjective(Engine& engineContext) {
    g_clueNotes.clear();
    g_foundNotes.clear();


    g_clueNotes.push_back(makeClueNote(engineContext,
        "Assistant's Regret",
        "I couldn't watch them suffer anymore. But the Director insists... stillness is purity. They only truly become art when they freeze.",
        7.0f, 2.9f));

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Torn Intake Log",
        "I found the paperwork for the newest acquisition. It's... it's you. The notes say you are 'conscious and disoriented'. Don't let them catch you.",
        3.6f, 6.6f));

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Last Journal Fragment",
        "You aren't escaping the museum... you're descending into the slaughterhouse it was built upon. The exhibits upstairs aren't statues. They're the ones who stopped moving.",
        9.3f, 8.1f));

    g_caveFinalNoteCollected = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    g_caveTimeoutActive = false;
    g_caveTimeoutTimer = 0.0f;
    g_caveTimeoutStage = 0;
    g_caveTimeoutFlashTimer = 0.0f;
    g_caveTimeoutTypedChars = 0;
    g_caveTimeoutTypingAccumulator = 0.0f;
    g_caveTimeoutCreditsTimer = 0.0f;
    initCaveQuiz();
}

static void clearPuzzleState() {
g_roomLocks.clear();
g_keyPickups.clear();
g_clueNotes.clear();
g_safes.clear();
g_symbols.clear();
g_safeBoxIndices.clear();
g_pedestalBoxIndices.clear();
g_playerKeys.clear();
g_foundNotes.clear();
g_accessPopup.clear();
g_accessPopupUntil = 0;
g_codeEntryActive = false;
g_codeEntryLockIndex = -1;
g_safeEntryIndex = -1;
g_symbolEntryIndex = -1;
g_codeEntryBuffer.clear();
    g_notesOpen = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    g_caveQuiz.clear();
    g_mindTrapActive = false;
    g_mindTrapTriggerConsumed = false;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;
    g_mindTrapLastHoveredOption = -1;
    g_mindTrapHoverTimer = 0.0f;
    g_mindTrapForcedCorrectionActive = false;
    g_mindTrapForcedCorrectionTimer = 0.0f;
    g_mindTrapForcedOption = -1;
    g_mindTrapChoiceJitterTimer = 0.0f;
    g_mindTrapTearActive = false;
    g_mindTrapTearTimer = 0.0f;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_solventLabUnlockCutsceneActive = false;
    g_solventLabUnlockCutsceneStage = 0;
    g_solventLabUnlockCutsceneTimer = 0.0f;
    g_solventLabUnlockTurnStartYaw = 0.0f;
    g_solventLabUnlockWhiteFlash = 0.0f;
    if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
    {
        g_worldModels[ g_solventLabMonsterModelIndex ].visible = false;
    }
    g_solventLabMonsterModelIndex = -1;
    g_museumPuzzleInitialized = false;
    g_caveTimerActive = false;
    g_caveTimeoutActive = false;
    g_caveTimeoutTimer = 0.0f;
    g_caveTimeoutStage = 0;
    g_caveTimeoutFlashTimer = 0.0f;
    g_caveTimeoutTypedChars = 0;
    g_caveTimeoutTypingAccumulator = 0.0f;
    g_caveTimeoutCreditsTimer = 0.0f;
    g_restorationWingUnlocked = false;
    g_directorDeskUnlocked = false;
    g_revolverPickup = {};
    g_combatState = {};
    g_showHeldWeapon = true;
    g_heldRevolverModelIndex = -1;
    g_revolverAiming = false;
    g_revolverShotCooldown = 0.0f;
    g_revolverRecoilTimer = 0.0f;
    g_revolverInspectCutsceneActive = false;
    g_revolverInspectCutsceneTimer = 0.0f;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    resetWhisperAmbience();
    g_firstLockedDoorDialogueShown = false;
    g_generatorNeedsGasLinePlayed = false;
    g_gasCanCollected = false;
    g_generatorFueled = false;
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;
    g_powerRestoreFlickerActive = false;
    g_powerRestoreFlickerTimer = 0.0f;
    g_solventCoolerEntryActive = false;
    g_solventCoolerBuffer.clear();
    g_redPigmentDispensed = false;
    g_redPigmentDispenseCutsceneActive = false;
    g_redPigmentDispenseCutsceneTimer = 0.0f;
    g_redPigmentDispenseModelIndex = -1;
    g_redPigmentDispenseBaseYaw = 0.0f;
    g_wakeCutsceneActive = false;
    g_wakeCutsceneTimer = 0.0f;
    g_wakeCutsceneStage = 0;
    g_wakeCutsceneStageTimer = 0.0f;
    g_wakeCutsceneTurnStartYaw = 0.0f;
    g_wakeDarknessOverride = -1.0f;
    if (g_wakeGeneratorHumSound)
    {
        g_wakeGeneratorHumSound->stop();
        g_wakeGeneratorHumSound.reset();
    }
    if (g_wakeMonsterModelIndex >= 0 && g_wakeMonsterModelIndex < (int)g_worldModels.size())
    {
        g_worldModels[ g_wakeMonsterModelIndex ].visible = false;
    }
    g_wakeMonsterModelIndex = -1;
    g_dialogue.clear();
    g_cutsceneController.reset();
}

static int findDoorLockIndex( Levels level, int tx, int ty ) {
    for (int i = 0; i < (int)g_roomLocks.size(); ++i)
    {
        if (g_roomLocks[ i ].level == level && g_roomLocks[ i ].tx == tx && g_roomLocks[ i ].ty == ty) return i;
    }
    return -1;
}

static Uint32 keyColorForName( const std::string &keyName ) {
    if (keyName == "BRONZE KEY") return rgb( 180, 120, 40 );
    if (keyName == "SILVER KEY") return rgb( 190, 190, 200 );
    if (keyName == "GOLD KEY") return rgb( 255, 215, 0 );
    if (keyName == "IRON KEY") return rgb( 135, 145, 155 );
    if (keyName == "BLACK PIGMENT") return rgb( 70, 70, 85 );
    if (keyName == "BLUE PIGMENT") return rgb( 90, 140, 220 );
    if (keyName == "RED PIGMENT") return rgb( 215, 75, 70 );
    if (keyName == "DIRECTOR'S KEY") return rgb( 210, 170, 95 );
    return rgb( 220, 210, 180 );
}

static std::string keyModelForName( const std::string &keyName ) {
    if (keyName == "BRONZE KEY") return "Bronze Key.glb";
    if (keyName == "SILVER KEY") return "Silver Key.glb";
    if (keyName == "GOLD KEY") return "Gold Key.glb";
    if (keyName == "IRON KEY") return "Iron Key.glb";
    if (keyName == "BLACK PIGMENT") return "BlackPigment.glb";
    if (keyName == "BLUE PIGMENT") return "BluePigment.glb";
    if (keyName == "RED PIGMENT") return "RedPigment.glb";
    if (keyName == "DIRECTOR'S KEY") return "Bronze Key.glb";
    return "Note.glb";
}

static bool isPlayerNearGenerator( Engine const &engineContext, float radius = 1.25f ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;
    return isPlayerNearPoint( engineContext, kMuseumGeneratorX, kMuseumGeneratorY, radius );
}

static bool isPlayerNearGasCan( Engine const &engineContext, float radius = 0.95f ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;
    if (g_gasCanCollected || g_generatorFueled) return false;
    return isPlayerNearPoint( engineContext, kMuseumGasCanX, kMuseumGasCanY, radius );
}

static void rebuildMuseumPowerInteractablesForLevel( Engine &engineContext, Levels level ) {
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;

    if (level != Levels::MUSEUM) return;

    g_generatorModelIndex = addWorldModelInstance(
        resolveFirstExistingAsset( { "Generator.glb", "generator.glb", "PowerGenerator.glb", "AirConditioner.glb" } ),
        kMuseumGeneratorX,
        kMuseumGeneratorY,
        0.70f,
        g_generatorFueled ? rgb( 225, 225, 205 ) : rgb( 130, 130, 130 ),
        -3.13,
        0.0f,
        0.0f,
        false,
        0.0f,
        -0.10f );

    if (!g_generatorFueled && !g_gasCanCollected)
    {
        g_gasCanModelIndex = addWorldModelInstance(
            resolveFirstExistingAsset( { "GasCan.glb", "Gas Can.glb", "gascan.glb", "MopBucket.glb" } ),
            kMuseumGasCanX,
            kMuseumGasCanY,
            0.3f,
            rgb( 208, 58, 52 ),
            1.5f,
            0.0f,
            0.0f,
            false,
            0.9f,
            -0.05f );
    }
}
static void rebuildMuseumInteractableVisualsForLevel(Engine& engineContext, Levels level) {
    for (auto& k : g_keyPickups)
    {
        k.propIndex = -1;
        k.modelIndex = -1;
        if (k.collected || k.level != level) continue;

        // Force 'true' for spawnVisuals here!
        KeyPickup vis = addKeyPickupModelProxy(engineContext, k.keyName, k.x, k.y, keyColorForName(k.keyName), keyModelForName(k.keyName), k.level, k.modelHeightOffset, true);
        k.propIndex = vis.propIndex;
        k.modelIndex = vis.modelIndex;
    }

    for (auto& n : g_clueNotes)
    {
        n.propIndex = -1;
        n.modelIndex = -1;
        if (n.collected || n.level != level) continue;

        NotePickupVisual vis = addNotePickupModel(engineContext, n.x, n.y, n.title);
        n.propIndex = vis.propIndex;
        n.modelIndex = vis.modelIndex;
    }

    // Respawn safes, pedestals, and decor when re-entering the ground floor
    if (level == Levels::MUSEUM) {
        g_safeBoxIndices.clear();
        for (auto& s : g_safes) {
            addSafe3D(engineContext, s.x, s.y);
        }
        g_pedestalBoxIndices.clear();
        for (auto& s : g_symbols) {
            addPedestal3D(engineContext, s.x, s.y);
        }

        // Director room furnishing + decor models
        addWorldModelInstance(resolveAssetModelPath("Full Desk.glb"), 16.36f, 16.55f, 0.8f, rgb(170, 150, 130), 3.1415926f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Shelf.glb"), 18.6f, 14.2f, 0.8f, rgb(170, 160, 140), -1.5707963f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Couch.glb"), 17.5f, 16.7f, 0.8f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Boxes.glb"), 16.2f, 15.3f, 0.8f, rgb(184, 130, 98), -1.5707963f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Whiteboard.glb"), 17.5f, 17.f, 0.8f, rgb(116, 101, 60), -1.5707963, 0, 1.5707963, false, 0, 0.45f);
        addWorldModelInstance(resolveAssetModelPath("Refrigerator.glb"), 17.4f, 14.2f, 0.8f, rgb(116, 101, 60), 2.3415926, -0.03, 0, false, 0, -0.08f);
        addWorldModelInstance(resolveAssetModelPath("FileCabinet.glb"), 16.2f, 16.0f, 0.4f, rgb(69, 41, 34), 1.5707963f, 0, 0, false, 0, -0.05f);

        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);

        addWorldModelInstance(resolveAssetModelPath("Scattered Paper.glb"), 17.1f, 14.4f, 0.20f, rgb(220, 210, 182), -0.20f, 0, 0, false, 0, -0.05f);
    }

    if (g_revolverPickup.collected)
    {
        g_revolverPickup.modelIndex = -1;
    }
    else if (g_revolverPickup.level == level)
    {
        bool needsSpawn = true;
        if (g_revolverPickup.modelIndex >= 0 && g_revolverPickup.modelIndex < (int)g_worldModels.size())
        {
            needsSpawn = false;
        }

        if (needsSpawn)
        {
            g_revolverPickup.modelIndex = addWorldModelInstance(resolveFirstExistingAsset({ "Revolver.glb", "revolver.glb", "SurgicalKnife.glb" }), g_revolverPickup.x, g_revolverPickup.y, 0.18f, rgb(165, 170, 180), 0.0f, 0.0f, -1.5707963f, true, 1.0f, 0.20f);
        }
    }
    else
    {
        g_revolverPickup.modelIndex = -1;
    }

    rebuildMuseumPowerInteractablesForLevel(engineContext, level);
}

static bool getDoorAheadTile( Engine const &engineContext, int &tx, int &ty ) {
    float reach = 1.5f;
    tx = int( engineContext.positionX + engineContext.directionX * reach );
    ty = int( engineContext.positionY + engineContext.directionY * reach );
    if (tx < 0 || ty < 0 || tx >= engineContext.map.width || ty >= engineContext.map.height) return false;
    return engineContext.map.tiles[ ty * engineContext.map.width + tx ] == 2;
}

static int getNearbyKeyPickup(Engine const& engineContext, float radius = 0.9f) {
    float radiusSq = radius * radius;
    int closestIndex = -1;
    float closestDistSq = radiusSq + 1.0f; // Start larger than max allowed

    for (int i = 0; i < (int)g_keyPickups.size(); ++i) {
        const auto& k = g_keyPickups[i];
        if (k.collected || k.level != engineContext.currentLevel) continue;

        float dx = engineContext.positionX - k.x;
        float dy = engineContext.positionY - k.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= radiusSq && distSq < closestDistSq) {
            closestDistSq = distSq;
            closestIndex = i;
        }
    }
    return closestIndex;
}

static int getNearbyClueNote( Engine const &engineContext, float radius = 0.9f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_clueNotes.size(); ++i)
    {
        const auto &n = g_clueNotes[ i ];
        if (n.collected) continue;
        if (n.level != engineContext.currentLevel) continue;
        float dx = engineContext.positionX - n.x;
        float dy = engineContext.positionY - n.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static int getNearbySafe( Engine const &engineContext, float radius = 1.6f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_safes.size(); ++i)
    {
        const auto &s = g_safes[ i ];
        if (s.solved) continue;
        float dx = engineContext.positionX - s.x;
        float dy = engineContext.positionY - s.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static int getNearbySymbol( Engine const &engineContext, float radius = 1.6f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_symbols.size(); ++i)
    {
        const auto &s = g_symbols[ i ];
        if (s.solved) continue;
        float dx = engineContext.positionX - s.x;
        float dy = engineContext.positionY - s.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static void hideWorldModelsNear( float x, float y, float radius = 0.50f ) {
    const float radiusSq = radius * radius;
    for (auto &m : g_worldModels)
    {
        if (!m.visible) continue;
        float dx = m.x - x;
        float dy = m.y - y;
        if ((dx * dx + dy * dy) <= radiusSq)
        {
            m.visible = false;
        }
    }
}

static bool loadLevel( Engine &engineContext, const LevelDef &level ) {
    namespace fs = std::filesystem;

    // Clear per-level state
    engineContext.artworks.clear();
    engineContext.artImages.clear();
    engineContext.props.clear();
    engineContext.propImages.clear();
    engineContext.quads.clear();
    engineContext.benches3D.clear();
    g_worldModels.clear();
    g_wepingStatues.clear();
    g_heldRevolverModelIndex = -1;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_revolverAiming = false;
    g_revolverRecoilTimer = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    g_editorSelectedModel = -1;
	engineContext.hasWallCracks = false;
    engineContext.hasFloorCracks = false;
    engineContext.caveMode = false;
    engineContext.hasFloorPuddles = false;
    engineContext.hasFloorStains = false;
    engineContext.hasWallStains = false;
    engineContext.hasWallOverlay = false;


    fs::path folder = level.folder;
    g_currentLevelFolder = folder.string();
    g_currentEditorModelsFile = makeEditorModelsFileNameForLevel( level.levelId, level.mapFile );
    refreshEditorAssetCatalog( true );
    /*
    {
        BoxProp box;
        box.centerX = 7.4f; box.centerY = 4.6f;
        box.halfLength = 0.5f;   // 2.0m long
        box.halfDepth = 0.5f;  // 0.5m deep
        box.height = 0.15f; // 55cm tall
        box.angle = 3.14159265f;

        // Load textures (or reuse existing images)
        if (!box.sideTexure.loadBMP( (folder / "bench.bmp").string() ))
        {
            box.sideTexure.width = 64; box.sideTexure.height = 64; box.sideTexure.pixels.assign( 64 * 64, rgb( 139, 90, 43 ) );
        }

        box.legTexure = box.sideTexure; // fallback


        box.legHalf = 0.05f;
        box.legInsetLength = 0.05f;   // pull legs inward along length
        box.legInsetDepth = 0.05f;   // pull legs inward along depth

        engineContext.benches3D.push_back( std::move( box ) );

    }
    */
  

    engineContext.ambianceTint = level.ambianceTint;
    engineContext.ambianceMul = level.ambianceMul;
    engineContext.indoorShadeLinear = level.isMuseumFloor ? 0.12f : 0.14f;
    engineContext.indoorShadeQuadratic = level.isMuseumFloor ? 0.035f : 0.050f;
    engineContext.indoorShadeMin = level.isMuseumFloor ? 0.01f : 0.015f;

    fs::path mapPath = level.mapFile.empty() ? (folder / "map.txt") : (folder / level.mapFile);
    // Map (1=wall, D=door)
    if (!loadMap( mapPath.string(), engineContext.map )) return false;

    auto loadOrFallback = [&]( const fs::path &path, Image &img, Uint32 fill ) {
        if (!img.loadBMP( path.string() ))
        {
            img.width = 64; img.height = 64;
            img.pixels.assign( 64 * 64, fill );
        }
        };

    loadOrFallback( folder / "wall.bmp", engineContext.wallTex, rgb( 80, 80, 100 ) );
    engineContext.hasFloor = engineContext.floorTex.loadBMP( (folder / "floor.bmp").string() );
    engineContext.hasCeiling = engineContext.ceilTex.loadBMP( (folder / "ceiling.bmp").string() );
    (void)engineContext.doorTexture.loadBMP( (folder / "door.bmp").string() );

    // Props
    loadProps( (folder / "props.txt").string(), engineContext.props, engineContext.propImages, engineContext.quads );

    auto modelTintForKind = [&]( const std::string &kind )->Uint32 {
        if (kind == "PLANT") return rgb( 255, 255, 255 );
        if (kind == "TRASHCAN") return rgb( 122, 132, 142 );
        if (kind == "VASE1" || kind == "VASE2" || kind == "VASE3") return rgb( 182, 158, 120 );
        if (kind == "BENCH") return rgb( 126, 96, 64 );
        if (kind == "SAFE") return rgb( 132, 144, 156 );
        if (kind == "PEDESTAL") return rgb( 164, 156, 142 );
        return rgb( 168, 168, 172 );
    };

    auto targetHeightForKind = [&]( const std::string &kind, float sourceScale )->float {
        const float s = std::max( 0.2f, sourceScale );
        if (kind == "PLANT") return 0.96f * s;
        if (kind == "TRASHCAN") return 0.88f * s;
        if (kind == "VASE1" || kind == "VASE2" || kind == "VASE3") return 0.74f * s;
        if (kind == "BENCH") return 0.56f * s;
        return 0.70f * s;
    };

    for (auto &prop : engineContext.props)
    {
        if (!prop.prefersModel || prop.modelAssetPath.empty()) continue;
        addWorldModelInstance( prop.modelAssetPath, prop.x, prop.y, targetHeightForKind( prop.kind, prop.scale ), modelTintForKind( prop.kind ), 0, 0, 0, false, 0, -0.05f );
        prop.scale = 0.0f;
    }

    // Build spatial buckets for quads (by tile)
    engineContext.quadBuckets.assign( engineContext.map.width * engineContext.map.height, {} );
    for (int i = 0; i < (int)engineContext.quads.size(); ++i)
    {
        const auto &q = engineContext.quads[ i ];
        int tx = (int)std::floor( q.centerX );
        int ty = (int)std::floor( q.centerY );
        if ((unsigned)tx < (unsigned)engineContext.map.width && (unsigned)ty < (unsigned)engineContext.map.height)
        {
            engineContext.quadBuckets[ ty * engineContext.map.width + tx ].push_back( i );
        }
    }


    if (isMuseumLikeLevel( (Levels)level.levelId ))
    {
        bool museumFreshStart = !g_museumPuzzleInitialized;
        if (!g_museumPuzzleInitialized)
        {
            initMuseumPuzzle( engineContext );
        }

		loadColumns( (folder / "columns.txt").string(), engineContext );

        if (loadArtworks( (folder / "artworks.txt").string(), engineContext.artworks ))
        {
            attachArtworksToWalls( engineContext );
            engineContext.artImages.resize( engineContext.artworks.size() );
            for (size_t i = 0; i < engineContext.artworks.size(); ++i)
            {
                std::filesystem::path ip = engineContext.artworks[ i ].imagePath;
                if (!ip.is_absolute()) ip = folder / ip;   // resolve relative to level folder
                // Always ensure art valid texture to avoid crashes later
                loadImageOrFallback( ip.string(), engineContext.artImages[ i ], rgb( 220, 220, 220 ) );
            }
        }

        // Stair transition is now represented as a wall mural overlay near the upper entry point.
        /*
        {
            BoxProp box;
            box.centerX = 2.6f; box.centerY = 2.0f;
            box.halfLength = 0.5f;   // 2.0m long
            box.halfDepth = 0.35f;  // 0.5m deep
            box.height = 0.15f; // 55cm tall
            box.angle = 3.14159265f;

            
            // Load textures (or reuse existing images)
            if (!box.sideTexure.loadBMP( (folder / "bench.bmp").string() ))
            {
                box.sideTexure.width = 64; box.sideTexure.height = 64; box.sideTexure.pixels.assign( 64 * 64, rgb( 139, 90, 43 ) );
            }

            box.legTexure = box.sideTexure; // fallback


            box.legHalf = 0.05f;
            box.legInsetLength = 0.05f;   // pull legs inward along length
            box.legInsetDepth = 0.05f;   // pull legs inward along depth

            engineContext.benches3D.push_back( std::move( box ) );

        }
        */

        if (museumFreshStart)
        {
            mesuemObjectives.viewedArtworks.clear();
            mesuemObjectives.totalArtworksToFind = (int)engineContext.artworks.size();
        }

        rebuildMuseumInteractableVisualsForLevel( engineContext, (Levels)level.levelId );
    }
    else
    {
        clearPuzzleState();
        if (level.levelId == Levels::CAVE)
        {
            initCaveFinalObjective( engineContext );
            g_caveTimerActive = true;
            g_caveTimerSeconds = 80.0f;
        }
    }

    engineContext.caveMode = (level.levelId == Levels::CAVE) || (level.levelId == Levels::TRANSITION);
    engineContext.hasWallOverlay = false;
    auto tryLoad = [&]( const std::filesystem::path &p, Image &dst, bool &flag ) {
        flag = dst.loadBMP( p.string() );
        };

    if (engineContext.caveMode)
    {
        // Optional overlay for rock variation
      //  std::filesystem::path overlay = (folder / "wall_overlay.bmp");
      //  if (engineContext.wallOverlay.loadBMP( overlay.string() ))
     //   {
     //       engineContext.hasWallOverlay = true;
     //   }
      

        engineContext.hasFloorCracks = engineContext.hasFloorStains = engineContext.hasFloorPuddles = false;
        engineContext.hasWallCracks = engineContext.hasWallStains = false;

        if (level.levelId == Levels::CAVE)
        {
            engineContext.lightRadius = 1.7f;
            engineContext.lightFalloff = 2.0f;
            engineContext.caveAmbient = 0.04f;

            tryLoad( folder / "floor_cracks.bmp", engineContext.floorOverlayCracks, engineContext.hasFloorCracks );
            //tryLoad( folder / "floor_stains.bmp", engineContext.floorOverlayStains, engineContext.hasFloorStains );
            tryLoad( folder / "floor_puddles.bmp", engineContext.floorOverlayPuddles, engineContext.hasFloorPuddles );

            tryLoad( folder / "wall_cracks.bmp", engineContext.wallOverlayCracks, engineContext.hasWallCracks );
            //tryLoad( folder / "wall_stain.bmp", engineContext.wallOverlayStains, engineContext.hasWallStains )
        }

        if (level.levelId == Levels::TRANSITION)
        {
            engineContext.lightRadius = 0.9f;
            engineContext.lightFalloff = 1.5f;
            engineContext.caveAmbient = 0.02f;
        }
    }

    if (g_unlockAllDoorsOverride)
    {
        applyUnlockAllDoorsOverride( engineContext );
    }

    loadEditorModelsForLevel();

    // Spawn & camera
    engineContext.positionX = level.spawnX;
    engineContext.positionY = level.spawnY;
    float art = level.spawnDirDeg * 3.14159265f / 180.f;
    engineContext.directionX = std::cos( art );
    engineContext.directionY = std::sin( art );
    engineContext.planeX = -engineContext.directionY * FOV_TAN;
    engineContext.planeY = engineContext.directionX * FOV_TAN;
    engineContext.yaw = level.spawnDirDeg;


    if (!level.objectiveLabel.empty())
    {
        mesuemObjectives.setMainObjective( level.objectiveLabel );
    }

    if (isMuseumLikeLevel( (Levels)level.levelId ))
    {
        if (mesuemObjectives.totalArtworksToFind <= 0)
        {
            mesuemObjectives.totalArtworksToFind = (int)engineContext.artworks.size();
        }
    }

    g_musicVolume = config::calibratedVolume;

    // Load the current levels' music track
    playMusicTrack( folder.string(), engineContext.currentLevel);

    return true;
}


static int pickArtworkUnderCrosshair( Engine const &engineContext ) {
    // Cast the same ray as the center column (x = RENDER_W / 2)
    int centerX = RENDER_W / 2;
    float camX = 2.0f * centerX / float( RENDER_W ) - 1.0f;
    float rayDirX = engineContext.directionX + engineContext.planeX * camX;
    float rayDirY = engineContext.directionY + engineContext.planeY * camX;

    int mapX = int( engineContext.positionX );
    int mapY = int( engineContext.positionY );
    float sideDistX, sideDistY;
    float deltaDistX = (rayDirX == 0 ? 1e30f : std::fabs( 1.0f / rayDirX ));
    float deltaDistY = (rayDirY == 0 ? 1e30f : std::fabs( 1.0f / rayDirY ));
    int stepX = 0;
    int stepY = 0;
    int side = 0;

    if (rayDirX < 0)
    {
        stepX = -1;
        sideDistX = (engineContext.positionX - mapX) * deltaDistX;
    }
    else
    {
        stepX = 1;
        sideDistX = (mapX + 1.0f - engineContext.positionX) * deltaDistX;
    }
    if (rayDirY < 0)
    {
        stepY = -1;
        sideDistY = (engineContext.positionY - mapY) * deltaDistY;
    }
    else
    {
        stepY = 1;
        sideDistY = (mapY + 1.0f - engineContext.positionY) * deltaDistY;
    }

    int hitTile = 0;
    while (!hitTile)
    {
        if (sideDistX < sideDistY)
        {
            sideDistX += deltaDistX; mapX += stepX; side = 0;
        }
        else
        {
            sideDistY += deltaDistY; mapY += stepY; side = 1;
        }
        if (mapX < 0 || mapY < 0 || mapX >= engineContext.map.width || mapY >= engineContext.map.height) return -1;
        hitTile = engineContext.map.tiles[ mapY * engineContext.map.width + mapX ];
    }
    if (hitTile != 1) return -1; // only real walls host framed art

    float perpWallDist = (side == 0)
        ? ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX)
        : ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
    perpWallDist = std::max( std::fabs( perpWallDist ), 0.05f );
   
    float wallX = (side == 0) ? (engineContext.positionY + perpWallDist * rayDirY)
        : (engineContext.positionX + perpWallDist * rayDirX);
    wallX -= std::floor( wallX );

    if (perpWallDist > 20.0f) return -1;

    int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );
    int yCenter = RENDER_H / 2;

    for (size_t artIndex = 0; artIndex < engineContext.artworks.size(); ++artIndex)
    {
        const auto &art = engineContext.artworks[ artIndex ];
        if (!art.onWall) continue;
        if (art.wx != mapX || art.wy != mapY || art.side != side) continue;

        float u0 = std::clamp( art.uCenter - 0.5f * art.uWidth, 0.0f, 1.0f );
        float u1 = std::clamp( art.uCenter + 0.5f * art.uWidth, 0.0f, 1.0f );
        if (wallX < u0 || wallX > u1) continue;

        int bandH = std::max( 1, int( lineH * art.vHeight ) );
        int bandCenter = RENDER_H / 2 + int( (art.vCenter - 0.5f) * lineH );
        int bandStart = std::clamp( bandCenter - bandH / 2, 0, RENDER_H - 1 );
        int bandEnd = std::clamp( bandStart + bandH - 1, 0, RENDER_H - 1 );

        if (yCenter >= bandStart && yCenter <= bandEnd)
        {
            return art.id; // This is the one under the crosshair
        }
    }
    return -1;
}

void handleLevelChange( Engine &engineContext, std::vector<LevelDef> levels, Levels desiredLevel ) {
    engineContext.currentLevel = desiredLevel;
    loadLevel( engineContext, levels[ desiredLevel ] );
    if (desiredLevel == Levels::MUSEUM_UPPER)
    {
        g_cutsceneController.triggerUpstairsGalleryCutscene( engineContext, g_dialogue );
    }
}

static void startWakeCutscene( Engine &engineContext ) {
    g_wakeCutsceneActive = true;
    g_wakeCutsceneTimer = 0.0f;
    g_wakeCutsceneStage = 0;
    g_wakeCutsceneStageTimer = 0.0f;
    g_wakeCutsceneInitialYaw = std::atan2( engineContext.directionY, engineContext.directionX );
    g_wakeCutsceneTurnStartYaw = g_wakeCutsceneInitialYaw;
    g_wakeCutsceneReturnStartYaw = g_wakeCutsceneInitialYaw;
    g_wakeDarknessOverride = 1.0f;
    if (g_wakeMonsterModelIndex >= 0 && g_wakeMonsterModelIndex < (int)g_worldModels.size())
    {
        g_worldModels[ g_wakeMonsterModelIndex ].visible = false;
    }
    g_wakeMonsterModelIndex = -1;
    if (g_wakeGeneratorHumSound)
    {
        g_wakeGeneratorHumSound->stop();
        g_wakeGeneratorHumSound.reset();
    }
    g_cutsceneController.clearHeadShake();
    g_revolverAiming = false;
    engineContext.pitchOffset = 30.0f;
}

static void startNewMuseumRun( Engine &engineContext, std::vector<LevelDef> levels ) {
    clearPuzzleState();
    g_notesCollectedRun = 0;
    g_runElapsedSeconds = 0.0f;
    handleLevelChange( engineContext, levels, Levels::MUSEUM );
    startWakeCutscene( engineContext );
}

static std::string normalizeMindTrapTerminalText( std::string s );

static void pushMindTrapTerminalLine( const std::string &line ) {
    g_mindTrapTerminalLog.push_back( normalizeMindTrapTerminalText( line ) );
    constexpr size_t kMaxTerminalLines = 120;
    if (g_mindTrapTerminalLog.size() > kMaxTerminalLines)
    {
        g_mindTrapTerminalLog.erase( g_mindTrapTerminalLog.begin(), g_mindTrapTerminalLog.begin() + (g_mindTrapTerminalLog.size() - kMaxTerminalLines) );
    }
}

static std::string normalizeMindTrapTerminalText( std::string s ) {
    for (char &c : s)
    {
        c = char( std::toupper( unsigned char( c ) ) );
        const bool ok =
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '.' || c == ',' || c == '-' || c == '_' || c == '/' ||
            c == '>' || c == '<' || c == '[' || c == ']' || c == '(' || c == ')' ||
            c == ':' || c == '?' || c == '!' || c == '\'' || c == ';' || c == '=' || c == '\\' ||
            c == '|' || c == '*' || c == '#' || c == '$';
        if (!ok) c = ' ';
    }
    return s;
}

static void queueMindTrapTerminalLine( const std::string &line ) {
    g_mindTrapTypeQueue.push_back( normalizeMindTrapTerminalText( line ) );
}

static void queueMindTrapPhasePrompt() {
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;

    const MindTrapPhase &phase = g_mindTrapPhases[ g_mindTrapPhaseIndex ];
    queueMindTrapTerminalLine("------------------------------------------------------------");
    queueMindTrapTerminalLine("VOICE> " + phase.prompt);
    queueMindTrapTerminalLine("THOUGHT> " + phase.options[0]); 
    queueMindTrapTerminalLine("THOUGHT> " + phase.options[1]);
    queueMindTrapTerminalLine("THOUGHT> " + phase.options[2]);
    queueMindTrapTerminalLine("VOICE>");

    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = std::clamp( g_mindTrapSelectedOption, 0, 2 );
}

static bool mindTrapTypewriterIdle() {
    return g_mindTrapTypeQueue.empty() && g_mindTrapTypingLine.empty();
}

static bool isMindTrapDenialOption( const std::string &optionText ) {
    const std::string opt = normalizeMindTrapTerminalText( optionText );
    return
        opt.find( "NO" ) != std::string::npos ||
        opt.find( "HELP" ) != std::string::npos ||
        opt.find( "WHO" ) != std::string::npos ||
        opt.find( "FAMILY" ) != std::string::npos ||
        opt.find( "NAME" ) != std::string::npos ||
        opt.find( "DIE" ) != std::string::npos ||
        opt.find( "RUN" ) != std::string::npos ||
        opt.find( "BREAK" ) != std::string::npos ||
        opt.find( "SCREAM" ) != std::string::npos ||
        opt.find( "BLINK" ) != std::string::npos ||
        opt.find( "STILL HERE" ) != std::string::npos;
}

static std::string mindTrapCorruptOptionText( std::string text, float intensity ) {
    if (intensity <= 0.02f) return text;

    auto replaceOnce = [&]( const std::string &from, const std::string &to ) {
        const size_t at = text.find( from );
        if (at != std::string::npos)
        {
            text.replace( at, from.size(), to );
        }
    };

    replaceOnce( "HELP", "CONFESS" );
    replaceOnce( "NO", "YES" );
    replaceOnce( "MY", "THE" );
    replaceOnce( "I ", "HE " );
    replaceOnce( "ME", "IT" );

    if (intensity > 0.46f)
    {
        text = "[ " + text + " ]";
    }
    if (intensity > 0.72f)
    {
        text = "INTERPRETATION: " + text;
    }

    return normalizeMindTrapTerminalText( text );
}

static int chooseMindTrapForcedOption( const MindTrapPhase &phase ) {
    int fallback = 0;
    for (int i = 0; i < 3; ++i)
    {
        const bool denial = isMindTrapDenialOption( phase.options[ i ] );
        if (denial) fallback = i;
    }

    if (phase.surrenderOption >= 0 && phase.surrenderOption < 3)
    {
        return phase.surrenderOption;
    }

    return std::clamp( fallback, 0, 2 );
}

static void updateMindTrapTypewriter( float dt ) {
    if (g_mindTrapPostLinePause > 0.0f)
    {
        g_mindTrapPostLinePause = std::max( 0.0f, g_mindTrapPostLinePause - dt );
        return;
    }

    if (g_mindTrapTypingLine.empty())
    {
        if (g_mindTrapTypeQueue.empty()) return;
        g_mindTrapTypingLine = g_mindTrapTypeQueue.front();
        g_mindTrapTypeQueue.pop_front();
        g_mindTrapTypingChars = 0;
        g_mindTrapTypingAccumulator = 0.0f;
    }

    const float pulse = 0.87f + 0.28f * std::max( 0.0f, std::sin( g_mindTrapFlickerTimer * 19.0f ) );
    g_mindTrapTypingAccumulator += dt * g_mindTrapTypingCharsPerSecond * pulse;

    auto charCost = []( char c ) -> float {
        if (c == ' ') return 0.55f;
        if (c == '.' || c == ',' || c == ':' || c == ';') return 2.1f;
        if (c == '?' || c == '!') return 2.5f;
        if (c == '/' || c == '\\' || c == '-' || c == '_' || c == '=') return 1.35f;
        return 1.0f;
    };

    while (g_mindTrapTypingChars < g_mindTrapTypingLine.size())
    {
        const float cost = charCost( g_mindTrapTypingLine[ g_mindTrapTypingChars ] );
        if (g_mindTrapTypingAccumulator < cost) break;
        ++g_mindTrapTypingChars;
        g_mindTrapTypingAccumulator -= cost;
    }

    if (g_mindTrapTypingChars >= g_mindTrapTypingLine.size())
    {
        pushMindTrapTerminalLine( g_mindTrapTypingLine );
        g_mindTrapTypingLine.clear();
        g_mindTrapTypingChars = 0;
        g_mindTrapTypingAccumulator = 0.0f;
        const bool shortStatusLine = g_mindTrapTerminalLog.empty() ? false : (g_mindTrapTerminalLog.back().size() <= 18);
        g_mindTrapPostLinePause = shortStatusLine ? 0.075f : 0.11f;
    }
}
static void initMindTrapPhases() {
    if (!g_mindTrapPhases.empty()) return;

    g_mindTrapPhases = {
        // Phase 0: The Hook
        {
            "HOW ARE YOU FEELING? DOES THE MARBLE BREATHE? ",
            { "I CAN SEE MY OWN BREATH", "IT'S SO COLD I CAN'T MOVE", "WHO ARE YOU?" },
            {
                "[ I AM SO COLD ]",
                "[ I CANNOT MOVE ]",
                "[ WHAT IS HAPPENING ]"
            },
            { "...", "...", "..." },
            -1
        },

        // Phase 1: The Horrific Realization
        {
            "THINK OF THE HORSE, THE STAG, THE WOLF. DID THEY LOOK SCARED TO YOU? DID THEY LOOK REAL?",
            { "THEY'RE JUST SOME STATUES", "IT WAS CREEPY, THEY WERE JUST LOOKING AT ME", "I THINK THEY LOOKED VERY REAL" },
            {
                "[ JUST STATUES ]",
                "[ THEY STARED AT ME ]",
                "[ THEY ARE REAL ]"
            },
            { "...", "...", "..." },
            -1
        },

        // Phase 2: The Eternity Clause
        {
            "CENTURIES WILL PASS. THE MUSEUM WILL CRUMBLE. BUT YOU WILL REMAIN CONSCIOUS.",
            { "PLEASE NO, I WILL GO MAD", "PLEASE LET ME DIE", "ME? I WON'T BE ALIVE IN CENTURIES" },
            {
                "[ I WILL GO MAD ]",
                "[ LET ME DIE ]",
                "[ I AM NOT A STATUE ]"
            },
            { "...", "...", "..." },
            -1
        },

        // Phase 3: Reveal 1 - The Architect
        {
            "WHO FILLED THE VATS? WHO POURED THE SOLVENT? LOOK AT YOUR HANDS.",
            { "THE DIRECTOR DID, THE DOCTORS DID, THOSE MONSTERS DID", "THE MONSTER", "I DID" },
            {
                "[ THE DIRECTOR DID ]",
                "[ A MONSTER ]",
                "[ ...I DID ]"
            },
            { "...", "...", "..." },
            -1
        },

        // Phase 4: Reveal 2 - The Masterpiece
        {
            "THE FINAL PEDESTAL IS EMPTY. IT WAS ALWAYS SAVED FOR YOU, DIRECTOR.",
            { "FINAL.NO", "FINAL.WAKE", "FINAL.SUBMIT" },
            {
                "[ NO... ]",
                "[ I NEED TO WAKE UP ]",
                "[ SUBMIT TO THE STONE ]"
            },
            {
                "PERFECT STILLNESS.",
                "PERFECT STILLNESS.",
                "PERFECT STILLNESS."
            },
            2 // Surrender option triggers the finale
        }
    };
}



static bool isPlayerNearMindTrapTrigger( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM_UPPER) return false;

    const bool nearSealedDoor = isPlayerNearPoint( engineContext, 16.4f, 9.4f, 1.25f );
    const bool nearSolventVault = isPlayerNearPoint( engineContext, 11.2f, 14.4f, 1.15f );
    return nearSealedDoor || nearSolventVault;
}

static void startMindTrapSequence( Engine &engineContext );

static void startSolventLabUnlockCutscene( Engine &engineContext ) {
    if (g_solventLabUnlockCutsceneActive) return;

    g_solventLabUnlockCutsceneActive = true;
    g_solventLabUnlockCutsceneStage = 0;
    g_solventLabUnlockCutsceneTimer = 0.0f;
    g_solventLabUnlockWhiteFlash = 0.0f;
    g_solventLabUnlockTurnStartYaw = std::atan2( engineContext.directionY, engineContext.directionX );

    if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
    {
        g_worldModels[ g_solventLabMonsterModelIndex ].visible = false;
    }
    g_solventLabMonsterModelIndex = -1;

    const float rightX = -engineContext.directionY;
    const float rightY = engineContext.directionX;
    const float spawnX = engineContext.positionX - engineContext.directionX * 3.1f + rightX * 0.32f;
    const float spawnY = engineContext.positionY - engineContext.directionY * 3.1f + rightY * 0.32f;
    const float spawnYaw = std::atan2( engineContext.positionY - spawnY, engineContext.positionX - spawnX );
    g_solventLabMonsterModelIndex = addWorldModelInstance(
        resolveFirstExistingAsset( { "Monster.glb", "monster.glb" } ),
        spawnX,
        spawnY,
        1.02f,
        rgb( 130, 130, 130 ),
        spawnYaw,
        0.0f,
        0.0f,
        false,
        0.0f,
        -0.05f );

    g_notesOpen = false;
    g_caveQuizActive = false;
    g_codeEntryActive = false;
    g_interactionAnim.active = false;
    g_levelTransition.active = false;
    g_revolverAiming = false;

    g_dialogue.start( {
        {"What, the room is empty? Where is the monster", 2.0f}
        } );
}

static void updateSolventLabUnlockCutscene( Engine &engineContext, GameState &currentState, float dt ) {
    if (!g_solventLabUnlockCutsceneActive) return;

    auto setYaw = [&]( float yaw ) {
        engineContext.directionX = std::cos( yaw );
        engineContext.directionY = std::sin( yaw );
        engineContext.planeX = -engineContext.directionY * FOV_TAN;
        engineContext.planeY = engineContext.directionX * FOV_TAN;
        engineContext.yaw = yaw * (180.0f / 3.14159265f);
        if (engineContext.yaw > 360.0f) engineContext.yaw -= 360.0f;
        if (engineContext.yaw < 0.0f) engineContext.yaw += 360.0f;
    };

    g_solventLabUnlockCutsceneTimer += dt;

    if (g_solventLabUnlockCutsceneStage == 0)
    {
        engineContext.pitchOffset = 0.0f;
        if (g_solventLabUnlockCutsceneTimer >= 1.55f)
        {
            if (g_whisperBaseFolder != g_currentLevelFolder)
            {
                g_whisperBaseFolder = g_currentLevelFolder;
                g_whisperBufferReady = g_whisperBuffer.loadFromFile( g_currentLevelFolder + "\\whisper.wav" );
            }
            if (g_whisperBufferReady)
            {
                playWeaponBufferedSound( g_whisperBuffer, 14.5f );
            }

            g_solventLabUnlockCutsceneStage = 1;
            g_solventLabUnlockCutsceneTimer = 0.0f;
            g_solventLabUnlockTurnStartYaw = std::atan2( engineContext.directionY, engineContext.directionX );
        }
        return;
    }

    if (g_solventLabUnlockCutsceneStage == 1)
    {
        const float turnProgress = std::clamp( g_solventLabUnlockCutsceneTimer / 0.78f, 0.0f, 1.0f );
        const float turnEase = turnProgress * turnProgress * (3.0f - 2.0f * turnProgress);
        const float headDip = 4.0f * turnProgress * (1.0f - turnProgress);
        setYaw( g_solventLabUnlockTurnStartYaw + turnEase * 3.14159265f );
        engineContext.pitchOffset = headDip * 3.2f;

        bool reachedPlayer = false;
        if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
        {
            auto &monster = g_worldModels[ g_solventLabMonsterModelIndex ];
            monster.visible = true;

            const float targetX = engineContext.positionX + engineContext.directionX * 0.62f;
            const float targetY = engineContext.positionY + engineContext.directionY * 0.62f;
            const float dx = targetX - monster.x;
            const float dy = targetY - monster.y;
            const float dist = std::sqrt( dx * dx + dy * dy );

            if (dist > 0.0001f)
            {
                const float step = std::min( dist, 11.5f * dt );
                monster.x += (dx / dist) * step;
                monster.y += (dy / dist) * step;
            }

            monster.yaw = std::atan2( engineContext.positionY - monster.y, engineContext.positionX - monster.x );
            reachedPlayer = dist <= 0.16f;
        }

        if ((turnProgress >= 0.75f && reachedPlayer) || g_solventLabUnlockCutsceneTimer >= 1.35f)
        {
            g_dialogue.start( {
                {"W-w-what is THAT", 2.1f}
                } );
            g_solventLabUnlockCutsceneStage = 2;
            g_solventLabUnlockCutsceneTimer = 0.0f;
            engineContext.pitchOffset = 0.0f;
        }
        return;
    }

    if (g_solventLabUnlockCutsceneStage == 2)
    {
        const float stareProgress = std::clamp( g_solventLabUnlockCutsceneTimer / 0.70f, 0.0f, 1.0f );
        const float uDip = 4.0f * stareProgress * (1.0f - stareProgress);
        engineContext.pitchOffset = uDip * 0.95f;

        if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
        {
            auto &monster = g_worldModels[ g_solventLabMonsterModelIndex ];
            const float targetX = engineContext.positionX + engineContext.directionX * 0.54f;
            const float targetY = engineContext.positionY + engineContext.directionY * 0.54f;
            const float dx = targetX - monster.x;
            const float dy = targetY - monster.y;
            const float dist = std::sqrt( dx * dx + dy * dy );
            if (dist > 0.0001f)
            {
                const float step = std::min( dist, 6.2f * dt );
                monster.x += (dx / dist) * step;
                monster.y += (dy / dist) * step;
            }
            monster.yaw = std::atan2( engineContext.positionY - monster.y, engineContext.positionX - monster.x );
        }

        if (g_solventLabUnlockCutsceneTimer >= 0.70f)
        {
            g_solventLabUnlockCutsceneStage = 3;
            g_solventLabUnlockCutsceneTimer = 0.0f;
            engineContext.pitchOffset = 0.0f;
        }
        return;
    }

    if (g_solventLabUnlockCutsceneStage == 3)
    {
        const float flashProgress = std::clamp( g_solventLabUnlockCutsceneTimer / 0.95f, 0.0f, 1.0f );
        g_solventLabUnlockWhiteFlash = std::clamp( std::pow( flashProgress, 0.65f ), 0.0f, 1.0f );

        if (g_solventLabUnlockCutsceneTimer >= 1.0f)
        {
            g_solventLabUnlockCutsceneActive = false;
            g_solventLabUnlockCutsceneStage = 0;
            g_solventLabUnlockCutsceneTimer = 0.0f;
            g_solventLabUnlockWhiteFlash = 0.0f;
            engineContext.pitchOffset = 0.0f;

            if (g_solventLabMonsterModelIndex >= 0 && g_solventLabMonsterModelIndex < (int)g_worldModels.size())
            {
                g_worldModels[ g_solventLabMonsterModelIndex ].visible = false;
            }
            g_solventLabMonsterModelIndex = -1;

            startMindTrapSequence( engineContext );
            currentState = STATE_MIND_TRAP;
        }
    }
}

static void startRedPigmentDispenseCutscene( Engine &engineContext ) {
    if (g_redPigmentDispenseCutsceneActive) return;
    if (g_playerKeys.contains( "RED PIGMENT" )) return;

    g_redPigmentDispenseCutsceneActive = true;
    g_redPigmentDispenseCutsceneTimer = 0.0f;
    g_redPigmentDispenseBaseYaw = std::atan2( engineContext.directionY, engineContext.directionX ) + kRevolverFacingYawOffset;
    g_revolverAiming = false;
    g_solventCoolerEntryActive = false;

    g_dialogue.start( {
        {"...Dispensing requested solvent pigment.", 2.4f}
        } );
}

static void updateRedPigmentDispenseCutscene( Engine &engineContext, float dt ) {
    if (!g_redPigmentDispenseCutsceneActive) return;

    g_redPigmentDispenseCutsceneTimer += dt;
    engineContext.pitchOffset = 5.0f;

    const float px = engineContext.positionX + engineContext.directionX * 0.56f;
    const float py = engineContext.positionY + engineContext.directionY * 0.56f;

    if (g_redPigmentDispenseModelIndex < 0 || g_redPigmentDispenseModelIndex >= (int)g_worldModels.size())
    {
        g_redPigmentDispenseModelIndex = addWorldModelInstance(
            resolveAssetModelPath( "RedPigment.glb" ),
            px,
            py,
            0.10f,
            rgb( 215, 75, 70 ),
            g_redPigmentDispenseBaseYaw,
            0.10f,
            -0.14f,
            true,
            3.8f,
            0.42f );
    }

    if (g_redPigmentDispenseModelIndex >= 0 && g_redPigmentDispenseModelIndex < (int)g_worldModels.size())
    {
        auto &m = g_worldModels[ g_redPigmentDispenseModelIndex ];
        m.visible = true;
        m.x = px;
        m.y = py;
        m.heightOffset = 0.42f;
        m.yaw = g_redPigmentDispenseBaseYaw;
        m.pitch = 0.24f;
        m.roll = -0.14f;
        m.spinYaw = true;
        m.spinSpeed = 3.8f;
    }

    if (g_redPigmentDispenseCutsceneTimer >= 2.8f && !g_dialogue.isActive())
    {
        g_redPigmentDispenseCutsceneActive = false;
        g_redPigmentDispenseCutsceneTimer = 0.0f;
        engineContext.pitchOffset = 0.0f;

        if (g_redPigmentDispenseModelIndex >= 0 && g_redPigmentDispenseModelIndex < (int)g_worldModels.size())
        {
            g_worldModels[ g_redPigmentDispenseModelIndex ].visible = false;
        }
        g_redPigmentDispenseModelIndex = -1;

        g_playerKeys.insert( "RED PIGMENT" );
        g_redPigmentDispensed = true;
        showAccessPopup( "Dispensed RED PIGMENT.", 2200 );
        triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED RED PIGMENT", 0.75f );
    }
}

static void startMindTrapSequence(Engine& engineContext) {
	g_musicVolume = 0.0f;
    initMindTrapPhases();

    g_mindTrapActive = true;
    g_mindTrapTriggerConsumed = true;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;

    g_notesOpen = false;
    g_caveQuizActive = false;
    g_codeEntryActive = false;
    g_interactionAnim.active = false;
    g_levelTransition.active = false;
    g_revolverAiming = false;
    g_dialogue.clear();

    engineContext.placardOpen = false;
    engineContext.openArtId = -1;
    engineContext.statueChatActive = false;

    queueMindTrapTerminalLine("...it's so cold...");
    queueMindTrapTerminalLine("...i feel heavy...");
    queueMindTrapTerminalLine("...the air smells like chemicals...");
    queueMindTrapTerminalLine("...why can't I blink?...");
    queueMindTrapTerminalLine("");
    queueMindTrapTerminalLine("VOICE> HELLO.");
    queueMindTrapTerminalLine("");

    queueMindTrapPhasePrompt();
}

static void commitMindTrapChoice(int choiceIndex) {
    if (!g_mindTrapActive || g_mindTrapShowingResult || g_mindTrapReadyToExit || !g_mindTrapAwaitingChoice) return;
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;
    if (choiceIndex < 0 || choiceIndex >= 3) return;

    const MindTrapPhase& phase = g_mindTrapPhases[g_mindTrapPhaseIndex];
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = choiceIndex;
    g_mindTrapLastHoveredOption = -1;
    g_mindTrapHoverTimer = 0.0f;
    g_mindTrapForcedCorrectionActive = false;
    g_mindTrapForcedCorrectionTimer = 0.0f;
    g_mindTrapForcedOption = -1;

    pushMindTrapTerminalLine(normalizeMindTrapTerminalText("YOU> " + phase.options[choiceIndex]));

    g_mindTrapLastResult = phase.results[choiceIndex];
    queueMindTrapTerminalLine("MIND> " + g_mindTrapLastResult);

    g_mindTrapResultTimer = 15.0f;

    g_mindTrapShowingResult = true;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;

    const bool finalPhase = (g_mindTrapPhaseIndex == (int)g_mindTrapPhases.size() - 1);
    if (finalPhase)
    {
        if (choiceIndex == phase.surrenderOption)
        {
            g_mindTrapFinalizeAfterResult = true;
        }
    }
    else
    {
        g_mindTrapAdvanceAfterResult = true;
    }
}

static void updateMindTrapSequence( Engine &engineContext, std::vector<LevelDef> &levels, GameState &currentState, float dt ) {
    if (!g_mindTrapActive) return;

    g_mindTrapFlickerTimer += dt;
    g_mindTrapChoiceJitterTimer += dt;
    updateMindTrapTypewriter( dt );

    if (!g_mindTrapShowingResult && !g_mindTrapReadyToExit && g_mindTrapAwaitingChoice &&
        g_mindTrapPhaseIndex >= 0 && g_mindTrapPhaseIndex < (int)g_mindTrapPhases.size())
    {
        const MindTrapPhase &phase = g_mindTrapPhases[ g_mindTrapPhaseIndex ];
        g_mindTrapSelectedOption = std::clamp( g_mindTrapSelectedOption, 0, 2 );

        if (g_mindTrapSelectedOption != g_mindTrapLastHoveredOption)
        {
            g_mindTrapLastHoveredOption = g_mindTrapSelectedOption;
            g_mindTrapHoverTimer = 0.0f;
        }
        else
        {
            g_mindTrapHoverTimer += dt;
        }

        if (g_mindTrapForcedCorrectionActive)
        {
            g_mindTrapForcedCorrectionTimer -= dt;
            g_mindTrapSelectedOption = std::clamp( g_mindTrapForcedOption, 0, 2 );
            if (g_mindTrapForcedCorrectionTimer <= 0.0f)
            {
                g_mindTrapForcedCorrectionActive = false;
            }
        }
        else
        {
            const bool denialHover = isMindTrapDenialOption( phase.options[ g_mindTrapSelectedOption ] );
            const bool lateStage = g_mindTrapPhaseIndex >= 2;
            if (denialHover && lateStage && g_mindTrapHoverTimer >= 2.2f)
            {
                g_mindTrapForcedCorrectionActive = true;
                g_mindTrapForcedCorrectionTimer = 1.15f;
                g_mindTrapForcedOption = chooseMindTrapForcedOption( phase );
                g_mindTrapSelectedOption = g_mindTrapForcedOption;
                queueMindTrapTerminalLine( "[CRITICAL_LOGIC_FAIL: SUBJECT_MEMORY_CORRUPTED]" );
                queueMindTrapTerminalLine( "ARCHIVE> FORCED CORRECTION APPLIED." );
                g_mindTrapHoverTimer = 0.0f;
            }
        }
    }

    if (g_mindTrapShowingResult)
    {
        if (mindTrapTypewriterIdle())
        {
            g_mindTrapResultTimer -= dt;
        }

        if (g_mindTrapResultTimer <= 0.0f)
        {
            g_mindTrapShowingResult = false;

            if (g_mindTrapAdvanceAfterResult)
            {
                g_mindTrapPhaseIndex = std::min( g_mindTrapPhaseIndex + 1, (int)g_mindTrapPhases.size() - 1 );
                queueMindTrapTerminalLine( "" );
                queueMindTrapPhasePrompt();
            }
            else if (g_mindTrapFinalizeAfterResult)
            {
                g_mindTrapReadyToExit = true;
                g_mindTrapWhiteFlashTimer = 0.0f;
                g_mindTrapTearActive = true;
                g_mindTrapTearTimer = 0.0f;
                queueMindTrapTerminalLine( "ARCHIVE CORE UNSTABLE." );
            }
            else
            {
                queueMindTrapTerminalLine( "" );
                queueMindTrapPhasePrompt();
            }

            g_mindTrapAdvanceAfterResult = false;
            g_mindTrapFinalizeAfterResult = false;
        }
    }

    if (!g_mindTrapShowingResult && !g_mindTrapReadyToExit && mindTrapTypewriterIdle())
    {
        g_mindTrapAwaitingChoice = true;
    }

    if (g_mindTrapReadyToExit)
    {
        if (!g_mindTrapTearActive)
        {
            g_mindTrapTearActive = true;
            g_mindTrapTearTimer = 0.0f;
        }

        g_mindTrapTearTimer += dt;
        if (g_mindTrapTearTimer >= 2.25f)
        {
            g_mindTrapActive = false;
            g_mindTrapReadyToExit = false;
            g_mindTrapShowingResult = false;
            g_mindTrapWhiteFlashTimer = 0.0f;
            g_mindTrapTearActive = false;
            g_mindTrapTearTimer = 0.0f;

            handleLevelChange( engineContext, levels, Levels::TRANSITION );
            currentState = STATE_GAME;
        }
    }
}

static bool isPlayerNearStatue( Engine const &engineContext ) {
    return false;
}

static bool isPlayerNearCaveStatue( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::CAVE) return false;

    constexpr float statueX = 11.1f;
    constexpr float statueY = 9.5f;
    constexpr float tolerance = 1.2f;

    float dx = engineContext.positionX - statueX;
    float dy = engineContext.positionY - statueY;
    return (dx * dx + dy * dy) <= (tolerance * tolerance);
}

void renderStatueChatbox( Engine &engineContext ) {
    const int fontW = 11;
    const int fontH = 16;
    const int letterSpacing = 1; // Used for header/main text
    const int lineSpacing = 2;   // Used for header/main text
    const int advY = fontH + lineSpacing; // Vertical advance for regular text

    int width = RENDER_W - 30, height = (RENDER_H / 4) - 40;
    int x = 7, y = RENDER_H / 2;

    drawTextBox( engineContext, x, y, width, height, rgb( 18, 18, 24 ), rgb( 90, 90, 120 ) );

    int textX = x + 8;
    int textY = y + 8;
    int textWidth = width - 16;

    std::string header = "ChatGPT Statue | OpenAI | Current | Relief Sculpture | MicroMuseum \n";

    drawString16x16( engineContext, textX, textY, header, rgb( 255, 255, 0 ), textWidth, letterSpacing, lineSpacing, true );

    textY += 2 * advY;

    std::string actualText = "";
    if (mesuemObjectives.allCompleted() == false)
    {
        actualText = "Thank you for exploring the museum! Please view all works, then come back!";
    }
    else
    {
        actualText = "All works logged. Proceed to the upper gallery diagnostic terminal.";
    }

    drawString16x16( engineContext, textX, textY, actualText, rgb( 210, 210, 210 ), textWidth, letterSpacing, lineSpacing, true );


    const int hintLetterSpacing = 0;
    const int hintLineSpacing = 5;
    const int hintAdvX = fontW + hintLetterSpacing; 

    std::string hint = "Press E to close";

    int hintX = x + width - (hint.length() * hintAdvX) - 40;

    int hintY = y + height - fontH - 4; // fontH = 16 (height of the text)

    drawString16x16( engineContext, hintX, hintY, hint, rgb( 150, 200, 255 ), textWidth, hintLetterSpacing, hintLineSpacing, true, rgb( 20, 20, 50 ) );
}


void updateMusicStream() {
    // Don't do anything if music was never started
    if (!g_musicInitialized)
    {
        return;
    }
    if (config::useMusic == false)
    {
        if (music.getStatus() != sf::SoundStream::Status::Stopped)
        {
            music.stop();
        }
        return;
    }

    // Check if the song has finished playing
    if (music.getStatus() == sf::SoundStream::Status::Stopped)
    {
        // The song finished! Play the next one.
        playNextTrack();
    }
}

void renderPolishedPlacard(Engine& engineContext) {
    if (!engineContext.placardOpen || engineContext.openArtId < 0) return;

    int artIndex = -1;
    for (size_t i = 0; i < engineContext.artworks.size(); ++i)
    {
        if (engineContext.artworks[i].id == engineContext.openArtId)
        {
            artIndex = (int)i;
            break;
        }
    }

    // If we somehow didn't find it, bail out
    if (artIndex < 0) return;

    const auto& art = engineContext.artworks[artIndex];

    int panelW = (int)(RENDER_W * 0.40f);
    int textMargin = 25;
    int maxTextW = panelW - (textMargin * 2);

    int estimatedHeight = 40; // Top padding
    estimatedHeight += 30;    // Title space
    estimatedHeight += 40;    // Meta/Location space
    estimatedHeight += 20;    // Divider

    auto calcH = [&](const std::string& t) {
        int lines = ((int)t.length() * 5 / maxTextW) + 1; // 5px per char approx
        return lines * 13;
        };

    estimatedHeight += calcH(art.placard);
    estimatedHeight += calcH(art.rationale);
    estimatedHeight += calcH(art.reflection);

    int panelH = std::min(estimatedHeight, RENDER_H - 40);
    int panelX = 20; // 20px gap from left edge
    int panelY = (RENDER_H - panelH) / 2;

    Uint32 bgCol = rgb(12, 12, 15);
    Uint32 borderCol = rgb(190, 160, 60);

    drawTranslucentBox(engineContext, panelX, panelY, panelW, panelH, bgCol, 0.90f);

    for (int x = panelX; x < panelX + panelW; ++x)
    {
        putPix(engineContext, x, panelY, borderCol);
        putPix(engineContext, x, panelY + panelH - 1, borderCol);
    }
    for (int y = panelY; y < panelY + panelH; ++y)
    {
        putPix(engineContext, panelX, y, borderCol);
        putPix(engineContext, panelX + panelW - 1, y, borderCol);
    }

    int currentY = panelY + textMargin;
    int textX = panelX + textMargin;

    drawString16x16(engineContext, textX, currentY, art.title, rgb(255, 255, 255), maxTextW, 1, 1, false);
    currentY += 25;

    std::string meta = art.artist + ", " + art.date;
    currentY = drawWrappedText(engineContext, textX, currentY, meta, borderCol, maxTextW);
    currentY += 3;

    currentY = drawWrappedText(engineContext, textX, currentY, art.location, rgb(150, 150, 150), maxTextW);
    currentY += 10;

    for (int x = textX; x < textX + maxTextW; ++x) putPix(engineContext, x, currentY, rgb(70, 70, 70));
    currentY += 15;

    currentY = drawWrappedText(engineContext, textX, currentY, art.placard, rgb(220, 220, 220), maxTextW);
    currentY += 20;

    drawStringTinyScaled(engineContext, textX, currentY, "HISTORICAL CONTEXT", borderCol, 1);
    currentY += 12;
    currentY = drawWrappedText(engineContext, textX, currentY, art.rationale, rgb(200, 200, 200), maxTextW);
    currentY += 20;

    drawStringTinyScaled(engineContext, textX, currentY, "ANALYSIS", borderCol, 1);
    currentY += 12;
    currentY = drawWrappedText(engineContext, textX, currentY, art.reflection, rgb(170, 190, 220), maxTextW);

    const Image& artImg = engineContext.artImages[artIndex];
    if (artImg.width > 0 && artImg.height > 0)
    {
        int availX = panelX + panelW + 20;
        int availW = RENDER_W - availX - 20;
        int availH = RENDER_H - 40;
        int availY = 20;

        float imgAspect = (float)artImg.width / (float)artImg.height;
        int drawW = availW;
        int drawH = (int)(drawW / imgAspect);

        if (drawH > availH)
        {
            drawH = availH;
            drawW = (int)(drawH * imgAspect);
        }

        int drawX = availX + (availW - drawW) / 2;
        int drawY = availY + (availH - drawH) / 2;

        for (int x = drawX - 1; x <= drawX + drawW; ++x)
        {
            putPix(engineContext, x, drawY - 1, borderCol);
            putPix(engineContext, x, drawY + drawH, borderCol);
        }
        for (int y = drawY - 1; y <= drawY + drawH; ++y)
        {
            putPix(engineContext, drawX - 1, y, borderCol);
            putPix(engineContext, drawX + drawW, y, borderCol);
        }

        for (int y = 0; y < drawH; ++y)
        {
            float v = (float)y / std::max(1.0f, (float)(drawH - 1));
            int texY = std::clamp((int)(v * artImg.height), 0, artImg.height - 1);

            for (int x = 0; x < drawW; ++x)
            {
                float u = (float)x / std::max(1.0f, (float)(drawW - 1));
                int texX = std::clamp((int)(u * artImg.width), 0, artImg.width - 1);

                Uint32 color = artImg.sample(texX, texY);

                if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255) continue;

                putPix(engineContext, drawX + x, drawY + y, color);
            }
        }
    }

}


void renderGalleryCard( Engine &engineContext ) {
    float px = engineContext.positionX;
    float py = engineContext.positionY;

    std::string wingName;
    std::string wingDesc;

    if (engineContext.currentLevel == Levels::MUSEUM_UPPER)
    {
        wingName = "Restoration Hub";
        wingDesc = "Research Facility";

        if (py < 7.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "Studio";
            wingDesc = "Specimen Processing";
        }
        else if (py > 12.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "Solvent Vault";
            wingDesc = "Storage";
        }
        else if (px < 7.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "Archive";
            wingDesc = "Corridor";
        }
        else if (px > 15.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "Bioroom";
            wingDesc = "Experimental";
        }
    }
    else
    {
        wingName = "Central Atrium";
        wingDesc = "Public Hub";

        if (py < 7.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "North Wing";
            wingDesc = "Newer";
        }
        else if (py > 12.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "South Wing";
            wingDesc = "Very, Very Old";
        }
        else if (px < 7.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "West Wing";
            wingDesc = "Very Old";
        }
        else if (px > 15.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "East Wing";
            wingDesc = "Modern";
        }
    }

    int titleW = (int)wingName.length() * 11;
    int titleX = (RENDER_W - titleW) / 2;

    int descW = (int)wingDesc.length() * 4;
    int descX = (RENDER_W - descW) / 2;

    int boxW = std::max( titleW + 40, descW + 40 );
    int boxX = (RENDER_W - boxW) / 2;

    // Draw the card at the top center
    drawTextBox( engineContext, boxX, 10, boxW, 40, rgb( 15, 15, 18 ), rgb( 180, 150, 50 ) );
    drawString16x16( engineContext, titleX, 15, wingName, rgb( 255, 230, 100 ), RENDER_W, 1, 1, true, rgb( 0, 0, 0 ) );
    drawStringTinyScaled( engineContext, descX, 35, wingDesc, rgb( 200, 200, 200 ), 1, 1, 1, false );
}


void renderObjectives( Engine &engineContext ) {
    int width = (RENDER_W / 3) + 20;
    int height = 55;
    int x = RENDER_W - width - 10; // Anchor to top right
    int y = 10;

    Uint32 colBg = rgb( 15, 15, 18 );
    Uint32 colBorder = rgb( 180, 150, 50 ); // Museum Gold
    Uint32 colText = rgb( 220, 220, 230 );

    // Draw main box
    drawTextBox( engineContext, x, y, width, height, colBg, colBorder );

    std::string header = "Progress";
    drawString16x16( engineContext, x + 10, y + 8, header, colBorder, width, 1, 1, false );

    // Draw Progress Bar outline
    int barX = x + 10;
    int barY = y + 30;
    int barWidth = width - 20;
    int barHeight = 12;
    drawTextBox( engineContext, barX, barY, barWidth, barHeight, rgb( 10, 10, 10 ), rgb( 100, 100, 100 ) );

    // Fill Progress Bar
    float progress = mesuemObjectives.getProgress();
    int fillWidth = (int)((barWidth - 2) * progress);
    for (int by = barY + 1; by < barY + barHeight - 1; ++by)
    {
        for (int bx = barX + 1; bx < barX + 1 + fillWidth; ++bx)
        {
            putPix( engineContext, bx, by, rgb( 180, 150, 50 ) ); // Gold fill
        }
    }

    // Progress Text
    std::string progText = std::to_string( mesuemObjectives.viewedArtworks.size() ) + "/" + std::to_string( mesuemObjectives.totalArtworksToFind );
    drawStringTinyScaled( engineContext, barX + barWidth - 30, barY - 12, progText, colText, 1 );

    if (!mesuemObjectives.mainObjective.empty())
    {
        drawStringTinyScaled( engineContext, x + 10, y + height - 8, mesuemObjectives.mainObjective, rgb( 190, 190, 205 ), 1, 1, 1, false );
    }
}

static void renderCaveHUD( Engine &engineContext ) {
    if (!g_caveTimerActive || engineContext.currentLevel != Levels::CAVE || g_caveQuizPassed) return;

    int w = 180;
    int h = 40;
    int x = RENDER_W - w - 15;
    int y = 10;
    drawTextBox( engineContext, x, y, w, h, rgb( 20, 10, 10 ), rgb( 180, 50, 50 ) );

    int mins = (int)g_caveTimerSeconds / 60;
    int secs = (int)g_caveTimerSeconds % 60;
    char buf[ 32 ];
    snprintf( buf, sizeof( buf ), "Escape %02d:%02d", mins, secs );
    drawString16x16( engineContext, x + 17, y + 12, buf, rgb( 255, 100, 100 ), w, 1, 1, false );
}

static void renderDialogueSubtitle( Engine &engineContext ) {
    if (!g_dialogue.isActive()) return;

    const std::string text = g_dialogue.currentText();
    if (text.empty()) return;

    int w = 760;
    int h = 50;
    int x = (RENDER_W - w) / 2;
    int y = RENDER_H - h - 30;

    drawTextBox( engineContext, x, y, w, h, rgb( 8, 8, 12 ), rgb( 130, 130, 150 ) );
    drawWrappedText( engineContext, x + 14, y + 18, text, rgb( 235, 235, 235 ), w - 28, 2);
}

static void renderCombatHUD( Engine &engineContext ) {
    if (!g_combatState.active || !g_combatState.hasRevolver) return;

    int w = 200;
    int h = 50;
    int x = 12;
    int y = RENDER_H - h - 12;

    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 150, 150, 165 ) );
    drawStringTinyScaled( engineContext, x + 12, y + 12, "Revolver", rgb( 220, 220, 225 ), 2, 1, 1, false );
    std::string ammo = std::to_string( g_combatState.loadedAmmo ) + " / " + std::to_string( g_combatState.reserveAmmo );
    drawString16x16( engineContext, x + 98, y + 20, ammo, rgb( 255, 215, 120 ), w - 104, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 34, "H To Holster", rgb( 170, 170, 185 ), 1, 1, 1, false );
}

static void renderHeldRevolver( Engine &engineContext ) {
    (void)engineContext;
}

static void renderRevolverShotEffects( Engine &engineContext, float intensity ) {
    const float shotFx = std::clamp( intensity, 0.0f, 1.0f );
    if (shotFx <= 0.001f) return;

    const float flash = std::pow( shotFx, 0.42f );
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 245, 230 ), 0.34f * flash );
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 210, 170 ), 0.12f * flash );

    const int cx = g_revolverAiming ? int( RENDER_W * 0.55f ) : int( RENDER_W * 0.66f );
    const int cy = g_revolverAiming ? int( RENDER_H * 0.58f ) : int( RENDER_H * 0.66f );
    const float radius = std::max( 38.0f, 185.0f * flash );

    for (int y = std::max( 0, cy - int( radius ) ); y <= std::min( RENDER_H - 1, cy + int( radius ) ); ++y)
    {
        for (int x = std::max( 0, cx - int( radius ) ); x <= std::min( RENDER_W - 1, cx + int( radius ) ); ++x)
        {
            const float dx = float( x - cx );
            const float dy = float( y - cy );
            const float d = std::sqrt( dx * dx + dy * dy );
            if (d > radius) continue;

            const float glow = std::clamp( 1.0f - (d / radius), 0.0f, 1.0f ) * flash;
            if (glow < 0.22f) continue;

            const Uint8 r = Uint8( std::clamp( 180.0f + 58.0f * glow, 0.0f, 255.0f ) );
            const Uint8 g = Uint8( std::clamp( 145.0f + 62.0f * glow, 0.0f, 255.0f ) );
            const Uint8 b = Uint8( std::clamp( 116.0f + 48.0f * glow, 0.0f, 255.0f ) );
            putPix( engineContext, x, y, rgb( r, g, b ) );
        }
    }
}

static void renderSchoolSafeWeaponBlur( Engine &engineContext ) {
    if (!config::schoolMode) return;
    if (!g_combatState.active || !g_combatState.hasRevolver) return;
    if (!g_showHeldWeapon && !g_revolverInspectCutsceneActive) return;

    int x = int( RENDER_W * 0.56f );
    int y = int( RENDER_H * 0.54f );
    int w = int( RENDER_W * 0.36f );
    int h = int( RENDER_H * 0.30f );

    if (g_revolverAiming)
    {
        x = int( RENDER_W * 0.45f );
        y = int( RENDER_H * 0.48f );
        w = int( RENDER_W * 0.26f );
        h = int( RENDER_H * 0.26f );
    }
    else if (g_revolverInspectCutsceneActive)
    {
        x = int( RENDER_W * 0.40f );
        y = int( RENDER_H * 0.35f );
        w = int( RENDER_W * 0.30f );
        h = int( RENDER_H * 0.35f );
    }

    x = std::clamp( x, 0, RENDER_W - 1 );
    y = std::clamp( y, 0, RENDER_H - 1 );
    w = std::clamp( w, 1, RENDER_W - x );
    h = std::clamp( h, 1, RENDER_H - y );

    const int block = 6;
    for (int by = y; by < y + h; by += block)
    {
        for (int bx = x; bx < x + w; bx += block)
        {
            const int ex = std::min( bx + block, x + w );
            const int ey = std::min( by + block, y + h );

            uint32_t sumR = 0, sumG = 0, sumB = 0, count = 0;
            for (int py = by; py < ey; ++py)
            {
                for (int px = bx; px < ex; ++px)
                {
                    Uint32 c = engineContext.backbuffer[ py * RENDER_W + px ];
                    sumR += (c >> 16) & 255;
                    sumG += (c >> 8) & 255;
                    sumB += c & 255;
                    ++count;
                }
            }

            if (count == 0) continue;
            const Uint32 avg = rgb( Uint8( sumR / count ), Uint8( sumG / count ), Uint8( sumB / count ) );

            for (int py = by; py < ey; ++py)
            {
                for (int px = bx; px < ex; ++px)
                {
                    putPix( engineContext, px, py, avg );
                }
            }
        }
    }

    drawTextBox( engineContext, x, y, w, h, rgb( 0, 0, 0 ), rgb( 120, 120, 140 ) );
}

static void renderAccessPopup( Engine &engineContext ) {
    if (g_accessPopup.empty() || SDL_GetTicks() > g_accessPopupUntil) return;
    if (g_dialogue.isActive()) return;

    int w = 500;
    int h = 70;
    int x = (RENDER_W - w) / 2;
    int y = RENDER_H - h - 20;
    bool denied = (g_accessPopup.find( "denied" ) != std::string::npos) ||
        (g_accessPopup.find( "Denied" ) != std::string::npos) ||
        (g_accessPopup.find( "required" ) != std::string::npos);
    Uint32 border = denied ? rgb( 200, 40, 40 ) : rgb( 120, 170, 70 );
    Uint32 head = denied ? rgb( 255, 80, 80 ) : rgb( 180, 230, 120 );
    std::string title = denied ? "It's Locked" : "LOG UPDATED";
    drawTextBox( engineContext, x, y, w, h, rgb( 12, 12, 16 ), border );
    drawString16x16( engineContext, x + 12, y + 10, title, head, w - 24, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 38, g_accessPopup, rgb( 230, 230, 230 ), 2, 1, 1, false );
 
}

static void renderCodeEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_codeEntryLockIndex < 0 || g_codeEntryLockIndex >= (int)g_roomLocks.size()) return;

    const RoomLock &lock = g_roomLocks[ g_codeEntryLockIndex ];
    int w = 420;
    int h = 150;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 180, 150, 50 ) );

    drawString16x16( engineContext, x + 16, y + 14, "ENTER ACCESS CODE", rgb( 255, 220, 120 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, lock.roomName, rgb( 170, 170, 185 ), 2, 1, 1, false );
    drawTextBox( engineContext, x + 16, y + 65, w - 32, 34, rgb( 0, 0, 0 ), rgb( 90, 90, 110 ) );
    drawString16x16( engineContext, x + 30, y + 74, g_codeEntryBuffer.empty() ? "----" : g_codeEntryBuffer, rgb( 220, 220, 230 ), w - 60, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 112, "TYPE 4 DIGITS, ENTER TO CONFIRM, ESC TO CANCEL", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static void renderSafeEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_safeEntryIndex < 0 || g_safeEntryIndex >= (int)g_safes.size()) return;

    const SafePuzzle &safe = g_safes[ g_safeEntryIndex ];
    int w = 380;
    int h = 160;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 15, 10, 10 ), rgb( 150, 150, 150 ) );

    drawString16x16( engineContext, x + 16, y + 14, "SAFE CODE", rgb( 210, 210, 210 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, safe.safeName, rgb( 170, 170, 185 ), 2, 1, 1, false );
    drawTextBox( engineContext, x + 16, y + 65, w - 32, 34, rgb( 0, 0, 0 ), rgb( 70, 70, 70 ) );
    drawString16x16( engineContext, x + 30, y + 74, g_codeEntryBuffer.empty() ? "----" : g_codeEntryBuffer, rgb( 220, 220, 230 ), w - 60, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 120, "INPUT 4 DIGITS, ENTER CONFIRM, ESC CANCEL", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static void renderSymbolEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_symbolEntryIndex < 0 || g_symbolEntryIndex >= (int)g_symbols.size()) return;

    const SymbolPuzzle &sym = g_symbols[ g_symbolEntryIndex ];
    int w = 460;
    int h = 200;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 15, 10 ), rgb( 100, 150, 100 ) );

    drawString16x16( engineContext, x + 16, y + 14, "PEDESTAL", rgb( 150, 220, 150 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, sym.name, rgb( 170, 170, 185 ), 2, 1, 1, false );

    const char* symbolNames[] = { "HORSE", "WOLF", "STAG", "SERPENT" };

    for(int i = 0; i < 3; ++i) {
        int bx = x + 30 + (i * 140);
        int by = y + 70;
        bool focus = (g_symbolFocus == i);
        drawTextBox( engineContext, bx, by, 120, 50, rgb( 5, 5, 5 ), focus ? rgb(200, 200, 200) : rgb( 60, 60, 60 ) );
        drawString16x16( engineContext, bx + 10, by + 16, symbolNames[g_symbolState[i]], focus ? rgb( 255, 255, 255 ) : rgb( 160, 160, 160 ), 100, 1, 1, false );
    }

    drawStringTinyScaled( engineContext, x + 16, y + 140, "LEFT/RIGHT SELECT SLOT", rgb( 120, 120, 140 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 155, "UP/DOWN CHANGE SYMBOL", rgb( 120, 120, 140 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 170, "ENTER TO SUBMIT, ESC TO EXIT", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static void renderSolventCoolerEntry( Engine &engineContext ) {
    if (!g_solventCoolerEntryActive) return;

    int w = 620;
    int h = 220;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;

    drawTextBox( engineContext, x, y, w, h, rgb( 8, 12, 12 ), rgb( 100, 170, 170 ) );
    drawString16x16( engineContext, x + 16, y + 14, "SOLVENT COOLER", rgb( 170, 235, 235 ), w - 32, 1, 1, false );

    int ry = y + 48;
    ry = drawWrappedText( engineContext, x + 16, ry, "I AM THE CURATOR'S CREED OF PERFECT STILLNESS. What's his phrase?", rgb( 195, 210, 210 ), w - 32 );
    ry += 8;

    drawTextBox( engineContext, x + 16, ry, w - 32, 44, rgb( 0, 0, 0 ), rgb( 65, 95, 105 ) );

    std::string typed = g_solventCoolerBuffer;
    if (((SDL_GetTicks() / 350) % 2) == 0) typed += "_";
    drawString16x16( engineContext, x + 24, ry + 13, typed.empty() ? "_" : typed, rgb( 230, 235, 235 ), w - 48, 1, 1, false );

    drawStringTinyScaled( engineContext, x + 16, y + h - 20, "TYPE PHRASE, ENTER CONFIRM, ESC CANCEL", rgb( 120, 150, 155 ), 1, 1, 1, false );
}

static std::vector<std::string> wrapNoteTextLines( const std::string &text, int maxCharsPerLine ) {
    std::vector<std::string> out;
    std::istringstream paragraphs( text );
    std::string paragraph;

    while (std::getline( paragraphs, paragraph, '\n' ))
    {
        if (paragraph.empty())
        {
            out.push_back( "" );
            continue;
        }

        std::istringstream words( paragraph );
        std::string word;
        std::string line;
        while (words >> word)
        {
            if (line.empty())
            {
                line = word;
            }
            else if ((int)(line.size() + 1 + word.size()) <= maxCharsPerLine)
            {
                line += " " + word;
            }
            else
            {
                out.push_back( line );
                line = word;
            }
        }
        if (!line.empty()) out.push_back( line );
    }

    if (out.empty()) out.push_back( "" );
    return out;
}

static void renderNotesScreen( Engine &engineContext ) {
    if (!g_notesOpen) return;

    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), 180.0f / 255.0f );




    int panelW = RENDER_W - 140;
    int panelH = RENDER_H - 110;
    int x = (RENDER_W - panelW) / 2;
    int y = (RENDER_H - panelH) / 2;

    drawTranslucentBox( engineContext, x + 8, y + 8, panelW, panelH, rgb( 0, 0, 0 ), 100.0f / 255.0f );
    drawTextBox( engineContext, x, y, panelW, panelH, rgb( 240, 240, 235 ), rgb( 212, 212, 206 ) );
    drawString16x16( engineContext, x + 18, y + 16, "NOTES", rgb( 30, 30, 30 ), panelW - 36, 1, 1, false );
  
    drawStringTinyScaled( engineContext, x + panelW - 290, y + 22, "UP/DOWN SELECT  PGUP/PGDN SCROLL  N/ESC CLOSE", rgb( 70, 70, 70 ), 1, 1, 1, false );

    int listX = x + 16;
    int listY = y + 52;
    int listW = 290;
    int listH = panelH - 66;

    int bodyX = listX + listW + 12;
    int bodyY = listY;
    int bodyW = panelW - (bodyX - x) - 16;
    int bodyH = listH;

    drawTextBox( engineContext, listX, listY, listW, listH, rgb( 232, 232, 227 ), rgb( 205, 205, 198 ) );
    drawTextBox( engineContext, bodyX, bodyY, bodyW, bodyH, rgb( 235, 235, 230 ), rgb( 205, 205, 198 ) );

    if (g_foundNotes.empty())
    {
        drawString16x16( engineContext, listX + 12, listY + 14, "No clues collected yet.", rgb( 30, 30, 30 ), listW - 24, 1, 1, false );
        return;
    }

    g_notesSelected = std::clamp( g_notesSelected, 0, (int)g_foundNotes.size() - 1 );
    int listLineY = listY + 10;
    const int listLineStep = 18;
    for (int i = 0; i < (int)g_foundNotes.size(); ++i)
    {
        int noteIdx = g_foundNotes[ i ];
        if (noteIdx < 0 || noteIdx >= (int)g_clueNotes.size()) continue;
        const auto &note = g_clueNotes[ noteIdx ];

        bool selected = (i == g_notesSelected);
        if (selected)
        {
            drawTextBox( engineContext, listX + 6, listLineY - 2, listW - 12, 16, rgb( 225, 225, 220 ), rgb( 30, 30, 30 ) );
        }

        drawStringTinyScaled( engineContext, listX + 10, listLineY, note.title, selected ? rgb( 30, 30, 30 ) : rgb( 55, 55, 55 ), 1, 1, 1, false );
        listLineY += listLineStep;
        if (listLineY > listY + listH - 14) break;
    }

    int selectedNoteIdx = g_foundNotes[ g_notesSelected ];
    if (selectedNoteIdx < 0 || selectedNoteIdx >= (int)g_clueNotes.size()) return;
    const auto& selected = g_clueNotes[selectedNoteIdx];
    drawString16x16(engineContext, bodyX + 12, bodyY + 10, selected.title, rgb(30, 30, 30), bodyW - 24, 1, 1, false);

    const int fontBaseWidth = 6;  // Base width of TinyScaled font at scale 1
    const int fontBaseHeight = 12; // Base line height at scale 1
    const int currentScale = 2;    // Your target scale

    const int maxChars = std::max(10, (bodyW - 24) / (fontBaseWidth * currentScale));

    std::vector<std::string> wrapped = wrapNoteTextLines(selected.body, maxChars);

    const int lineHeight = fontBaseHeight * currentScale;
    const int visibleLines = std::max(1, (bodyH - 50) / lineHeight);

    const int maxScroll = std::max(0, (int)wrapped.size() - visibleLines);
    g_notesBodyScroll = std::clamp(g_notesBodyScroll, 0, maxScroll);

    int textY = bodyY + 40; // Slightly more padding for the title
    for (int i = g_notesBodyScroll; i < (int)wrapped.size() && i < g_notesBodyScroll + visibleLines; ++i)
    {
        // Render with both X and Y scale set to 2 for consistency
        drawStringTinyScaled(engineContext, bodyX + 12, textY, wrapped[i], rgb(30, 30, 30), currentScale, currentScale, 1, false);
        textY += lineHeight; // Move down by the scaled line height
    }

    std::string scroll = "LINE " + std::to_string( std::min( (int)wrapped.size(), g_notesBodyScroll + 1 ) ) + "/" + std::to_string( std::max( 1, (int)wrapped.size() ) );
    drawStringTinyScaled( engineContext, bodyX + bodyW - 95, bodyY + bodyH - 14, scroll, rgb( 90, 90, 90 ), 1, 1, 1, false );
}

static void renderCompass( Engine &engineContext ) {
    const int boxX = 10;
    const int boxY = 10;
    const int boxW = 80;
    const int boxH = 80;
    const int cx = boxX + boxW / 2;
    const int cy = boxY + boxH / 2;
    const int r = 25;

    drawTextBox( engineContext, boxX, boxY, boxW, boxH, rgb( 14, 14, 18 ), rgb( 160, 140, 80 ) );
    for (int y = -r - 1; y <= r + 1; ++y)
    {
        for (int x = -r - 1; x <= r + 1; ++x)
        {
            int d2 = x * x + y * y;
            if (d2 >= (r - 1) * (r - 1) && d2 <= (r + 1) * (r + 1)) putPix( engineContext, cx + x, cy + y, rgb( 170, 150, 90 ) );
        }
    }
    drawStringTinyScaled( engineContext, cx - 2, cy - r - 10, "N", rgb( 235, 220, 170 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx - 2, cy + r + 4, "S", rgb( 180, 180, 180 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx + r + 5, cy - 2, "E", rgb( 180, 180, 180 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx - r - 8, cy - 2, "W", rgb( 180, 180, 180 ), 1, 1, 1, false );

    float ang = std::atan2( engineContext.directionY, engineContext.directionX );
    int nx = cx + int( std::cos( ang ) * (r - 6) );
    int ny = cy + int( std::sin( ang ) * (r - 6) );
    int x0 = cx, y0 = cy, x1 = nx, y1 = ny;
    int dx = std::abs( x1 - x0 ), sx = (x0 < x1) ? 1 : -1;
    int dy = -std::abs( y1 - y0 ), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (true)
    {
        putPix( engineContext, x0, y0, rgb( 230, 60, 60 ) );
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void renderLevelEditorOverlay( Engine &engineContext ) {
    if (!g_levelEditorMode) return;

    refreshEditorAssetCatalog();

    const auto &catalog = editorAssetCatalog();
    if (catalog.empty()) return;

    int w = 560;
    int h = 166;
    int x = 14;
    int y = RENDER_H - h - 14;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 90, 170, 210 ) );

    const auto &asset = catalog[ g_editorAssetIndex % (int)catalog.size() ];
    drawString16x16( engineContext, x + 12, y + 10, "LEVEL EDITOR MODE", rgb( 140, 215, 255 ), w - 24, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 34, "ASSET: " + asset.label + " (" + asset.assetName + ")", rgb( 220, 220, 220 ), 2, 1, 1, false );

    if (g_editorSelectedModel >= 0 && g_editorSelectedModel < (int)g_worldModels.size() && g_worldModels[ g_editorSelectedModel ].editorPlaced)
    {
        const auto &m = g_worldModels[ g_editorSelectedModel ];
        std::ostringstream tr;
        tr.setf( std::ios::fixed );
        tr.precision( 2 );
        tr << "SELECTED @ X " << m.x << "  Y " << m.y << "  Z " << m.heightOffset;
        drawStringTinyScaled( engineContext, x + 12, y + 50, tr.str(), rgb( 190, 220, 170 ), 2, 1, 1, false );

        std::ostringstream rot;
        rot.setf( std::ios::fixed );
        rot.precision( 2 );
        rot << "ROT Y/P/R " << m.yaw << " / " << m.pitch << " / " << m.roll << "  SIZE " << m.editorTargetScale;
        drawStringTinyScaled( engineContext, x + 12, y + 64, rot.str(), rgb( 190, 220, 170 ), 2, 1, 1, false );
    }
    else
    {
        drawStringTinyScaled( engineContext, x + 12, y + 50, "NO SELECTED MODEL", rgb( 200, 190, 150 ), 2, 1, 1, false );
    }

    drawStringTinyScaled( engineContext, x + 12, y + 88, "F2 TO EXIT [ ] CYCLE ASSET ENTER PLACE TAB SELECT DEL DELETE", rgb( 170, 170, 190 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 104, "MOUSE MOVE X/Y  WHEEL Z  |  WASD X/Y  R/F Z  Q/E YAW  Z/X PITCH  C/V ROLL  -/= SIZE", rgb( 170, 170, 190 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 120, "CTRL+S SAVE TO " + g_currentEditorModelsFile + " (current level)", rgb( 170, 170, 190 ), 1, 1, 1, false );
}


static void renderCaveTimeoutEndingScreen(Engine& engineContext) {
    drawTextBox(engineContext, 0, 0, RENDER_W, RENDER_H, rgb(0, 0, 0), rgb(0, 0, 0));

    static float localSequenceTimer = 0.0f;

    if (g_caveTimeoutStage == 1) {
        // Reset our local timer during the flash so it starts fresh every time
        localSequenceTimer = 0.0f;

        const float flashP = std::clamp(g_caveTimeoutFlashTimer / std::max(0.001f, kCaveTimeoutFlashDuration), 0.0f, 1.0f);
        const float alpha = std::clamp(1.0f - flashP, 0.0f, 1.0f);
        drawTranslucentBox(engineContext, 0, 0, RENDER_W, RENDER_H, rgb(255, 255, 255), alpha);
        return;
    }

    // Advance the local timer. 
    localSequenceTimer += 0.5f;
    int clock = (int)localSequenceTimer;

    // Colors
    const Uint32 ink = rgb(210, 210, 210);
    const Uint32 creditsInk = rgb(150, 160, 180);
    const Uint32 statueInk = rgb(110, 115, 125);
	const Uint32 finalLineInk = rgb(255, 0, 0);

    int topLineLen = (int)g_caveTimeoutLine.size();
    int topVisible = std::clamp(clock, 0, topLineLen);
    if (topVisible > 0) {
        std::string typedTopLine = g_caveTimeoutLine.substr(0, topVisible);
        drawString16x16(engineContext, 40, 40, typedTopLine, ink, RENDER_W - 80, 1, 1, false);
    }
    clock -= topLineLen; // Subtract to hold the sequence until this line finishes

    std::vector<std::string> statueArt = {
       "            *    .  *       ",
       "      .  *    *    *   .   ",
       "   *     .       .    *    ",
       "        )  *  *  *  (      ",
       "       .--\"\"\"\"\"--. ",
       "      /  .-. .-.  \\       ",
       "     |  (o ) (o )  |      ",
       "      \\   '---'   /       ",
       "       '-.___.-'--/         ",
       "         ||||             ",
       "    _____||||____         ",
       "   /    |||||    \\       ",
       "  / \\   | | |   / \\    ",
       " |   |  |   |  |   |      ",
       "o|   |  |   |  |   |o     ",
       "|    \\__|___|__/    |     ",
       " \\      [___]      /      ",
       "  \\    /     \\    /      ",
       "   \\  / ,   . \\  /       ",
       "    \\/  |   |  \\/        ",
       "         |   |             ",
       "        /|   |\\           ",
       "       /_|___|_\\          ",
       "  ___________________     ",
       " |===================|    ",
       " |       Y O U       |   ",
       " |===================|    ",
       "_|___________________|_   "
    };

    int statueY = 100;
    int statueX = 40;
    for (const auto& line : statueArt) {
        int lineLen = (int)line.size();
        int visible = std::clamp(clock, 0, lineLen);

        if (visible > 0) {
            std::string typedLine = line.substr(0, visible);
            drawStringTinyScaled(engineContext, statueX, statueY, typedLine, statueInk, 2, 1, 1, false);
        }

        statueY += 12;
        clock -= lineLen;
    }

    std::vector<std::string> credits = {
        "CREDITS",
        "Lines of code: 18,000",
        "Hours: 200 (4 months)",
        "Total Assets: 148",
        "Project Size: 19 GB",
        "Total Music Tracks: 8",
        "Total SFX: 13",
        "Play Time: 20-40 Mins",
        "",
        "Special Thanks:",
        "Google Gemini 3.1 Pro (coding)",
        "OpenAI ChatGPT-Codex (coding)"
    };

    int creditX = RENDER_W / 2 + 30;
    int creditY = 105;
    for (const auto& line : credits) {
        int lineLen = (int)line.size();
        // Add a small extra delay budget for empty strings so it pauses briefly on blanks
        if (lineLen == 0) lineLen = 5;

        int visible = std::clamp(clock, 0, lineLen);
        if (visible > 0) {
            std::string typedLine = line.substr(0, visible);
            drawStringTinyScaled(engineContext, creditX, creditY, typedLine, creditsInk, 2, 1, 1, false);
        }

        creditY += 18;
        clock -= lineLen;
    }

    std::string finalSentence = "You are finally ready for display.";
    int finalLen = (int)finalSentence.size();
    int finalVisible = std::clamp(clock, 0, finalLen);

    if (finalVisible > 0) {
        std::string typedFinal = finalSentence.substr(0, finalVisible);
        int finalX = (RENDER_W / 2) - (finalSentence.size() * 4); // Basic centering calculation
        drawString16x16(engineContext, finalX - 90, RENDER_H - 80, typedFinal, finalLineInk, RENDER_W, 1, 1, false);
    }
}

static void renderEndingScreen( Engine &engineContext ) {
    int w = RENDER_W - 120;
    int h = RENDER_H - 80;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 190, 160, 80 ) );
    drawString16x16( engineContext, x + 20, y + 20, "YOU FAILED TO ESCAPE", rgb( 255, 230, 120 ), w - 40, 1, 1, false );
    int cy = y + 60;
    cy = drawWrappedText( engineContext, x + 20, cy, "There was never an escape, it's all in your head. The museum was your brain's last ditch effort at salvation.", rgb( 220, 220, 220 ), w, 2);
    cy += 14;

    std::vector<std::string> statLines = {
        "Time: " + std::to_string(int(g_runElapsedSeconds)) + "s",
        "Total Lines of Code: 15,000",
        "Total Functions: 10,000",
        "Hours Spent: 200",
        "Total 3D Assets: 88",
        "Total Music Assets: 6",
        "Total Sound Effects: 11",
        "Total Art Assets: 20",
        "Total Sprites and 2D assets: 23",
        "Total Source Files: 28",
        "Total Assets: 148",
        "Total Project Size: 18.9 GB",
        "Total Files: 14,011",
        "Total Folders: 1,616",
        "Accelerated by ChatGPT-Codex and Gemini 1.5 Pro"
    };

    int currentY = cy;
    int lineSpacing = 15; 

    for (const std::string& line : statLines) {
        drawStringTinyScaled(engineContext, x + 20, currentY, line, rgb(170, 190, 220), 2, 1, 1, false);
        currentY += lineSpacing; // Move the next line down
    }

    // Draw the footer buttons as before
    drawString16x16(engineContext, x + 20, y + h - 34, "[R] Restart    [ESC] Menu", rgb(210, 210, 210), w - 40, 1, 1, false);
}

static int wrapTextureCoord( int value, int size ) {
    if (size <= 0) return 0;
    value %= size;
    if (value < 0) value += size;
    return value;
}

static void renderWorldModelsRange(
    Engine &engineContext,
    std::vector<float> &meshInvDepthBuffer,
    const std::vector<float> &wallInvDepthBuffer,
    float pitchOffset,
    int yStartInclusive,
    int yEndExclusive ) {
    if (yStartInclusive >= yEndExclusive) return;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );
    const float projScaleY = (RENDER_W * 0.5f);
    const float horizon = (RENDER_H * 0.5f) + pitchOffset;
    const float camHeight = 0.52f;
    const float nearClip = 0.18f;
    const float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
    const glm::vec3 lightDir = glm::normalize( glm::vec3( -0.35f, 0.85f, -0.40f ) );
    const float kViewPreload = 0.28f;
    const float kCullingEpsilon = 0.015f;

    struct CpuProjVert {
        float sx = 0.0f;
        float sy = 0.0f;
        float z = -1.0f;
        glm::vec3 world{0.0f};
        glm::vec3 color{1.0f};
        glm::vec2 uv{0.0f};
        bool valid = false;
    };

    auto toFixed256 = []( float v )->int {
        return std::clamp( int( v * 256.0f + 0.5f ), 0, 256 );
    };

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.model || inst.model->indices.size() < 3) continue;
        if (shouldGpuRenderModel( engineContext, inst )) continue;

        const glm::vec3 modelHalfExtents = glm::max( (inst.model->boundsMax - inst.model->boundsMin) * 0.5f, glm::vec3( 0.0001f ) ) * inst.scale;
        const float modelRadius = std::max( 0.05f, glm::length( modelHalfExtents ) );
        const float modelCenterY = inst.heightOffset + modelHalfExtents.y;

        const float centerDx = inst.x - engineContext.positionX;
        const float centerDy = inst.y - engineContext.positionY;
        const float centerTx = invDet * (engineContext.directionY * centerDx - engineContext.directionX * centerDy);
        const float centerTz = invDet * (-engineContext.planeY * centerDx + engineContext.planeX * centerDy);

        if ((centerTz + modelRadius) <= nearClip) continue;
        if ((centerTz - modelRadius) > g_worldModelRenderDistance) continue;

        const float txRadius = modelRadius / std::max( 0.001f, FOV_TAN );
        const float horizontalLimit = (1.0f + kViewPreload) * std::max( centerTz, nearClip );
        if ((centerTx - txRadius) > horizontalLimit || (centerTx + txRadius) < -horizontalLimit) continue;

        const float centerScreenY = horizon - ((modelCenterY - camHeight) * projScaleY / std::max( centerTz, nearClip ));
        const float verticalRadiusPx = (projScaleY * modelRadius) / std::max( centerTz, nearClip );
        if ((centerScreenY + verticalRadiusPx) < (-RENDER_H * kViewPreload) ||
            (centerScreenY - verticalRadiusPx) > (RENDER_H * (1.0f + kViewPreload)))
        {
            continue;
        }

        std::vector<CpuProjVert> projected( inst.model->vertices.size() );
        std::vector<glm::vec3> transformed( inst.model->vertices.size(), glm::vec3( 0.0f ) );

        const float timeSeconds = SDL_GetTicks() * 0.001f;
        const float yawNow = inst.yaw + (inst.spinYaw ? (inst.spinSpeed * timeSeconds) : 0.0f);
        const glm::quat qYaw = glm::angleAxis( yawNow, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        const glm::quat qPitch = glm::angleAxis( inst.pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        const glm::quat qRoll = glm::angleAxis( inst.roll, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        const glm::quat q = qYaw * qPitch * qRoll;
        const glm::vec3 pivot(
            (inst.model->boundsMin.x + inst.model->boundsMax.x) * 0.5f,
            inst.model->boundsMin.y,
            (inst.model->boundsMin.z + inst.model->boundsMax.z) * 0.5f );

        float modelMinY = std::numeric_limits<float>::max();
        for (size_t vi = 0; vi < inst.model->vertices.size(); ++vi)
        {
            const glm::vec3 local = (inst.model->vertices[ vi ] - pivot) * inst.scale;
            transformed[ vi ] = q * local;
            modelMinY = std::min( modelMinY, transformed[ vi ].y );
        }
        if (!std::isfinite( modelMinY )) modelMinY = 0.0f;

        bool hasProjectedVerts = false;
        float modelMinSx = std::numeric_limits<float>::max();
        float modelMaxSx = std::numeric_limits<float>::lowest();
        float modelMinSy = std::numeric_limits<float>::max();
        float modelMaxSy = std::numeric_limits<float>::lowest();
        float modelNearestZ = std::numeric_limits<float>::max();

        for (size_t vi = 0; vi < inst.model->vertices.size(); ++vi)
        {
            const glm::vec3 r = transformed[ vi ];

            const float wx = inst.x + r.x;
            const float wy = (r.y - modelMinY) + inst.heightOffset;
            const float wz = inst.y + r.z;

            const float dx = wx - engineContext.positionX;
            const float dy = wz - engineContext.positionY;
            const float tx = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            const float tz = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (tz <= nearClip) continue;

            projected[ vi ].sx = (RENDER_W * 0.5f) * (1.0f + (tx / tz));
            projected[ vi ].sy = horizon - ((wy - camHeight) * projScaleY / tz);
            projected[ vi ].z = tz;
            projected[ vi ].world = glm::vec3( wx, wy, wz );
            if (vi < inst.model->colors.size()) projected[ vi ].color = inst.model->colors[ vi ];
            if (vi < inst.model->uvs.size()) projected[ vi ].uv = inst.model->uvs[ vi ];
            projected[ vi ].valid = true;

            hasProjectedVerts = true;
            modelMinSx = std::min( modelMinSx, projected[ vi ].sx );
            modelMaxSx = std::max( modelMaxSx, projected[ vi ].sx );
            modelMinSy = std::min( modelMinSy, projected[ vi ].sy );
            modelMaxSy = std::max( modelMaxSy, projected[ vi ].sy );
            modelNearestZ = std::min( modelNearestZ, tz );
        }

        if (!hasProjectedVerts) continue;

        const int modelScreenMinX = std::max( 0, (int)std::floor( modelMinSx ) );
        const int modelScreenMaxX = std::min( RENDER_W - 1, (int)std::ceil( modelMaxSx ) );
        const int modelScreenMinY = std::max( 0, (int)std::floor( modelMinSy ) );
        const int modelScreenMaxY = std::min( RENDER_H - 1, (int)std::ceil( modelMaxSy ) );
        if (modelScreenMinX > modelScreenMaxX || modelScreenMinY > modelScreenMaxY) continue;
        if (modelScreenMaxY < yStartInclusive || modelScreenMinY >= yEndExclusive) continue;

        const int modelScreenW = modelScreenMaxX - modelScreenMinX + 1;
        const int modelScreenH = modelScreenMaxY - modelScreenMinY + 1;
        if (g_perfLowMode && (modelScreenW * modelScreenH) <= 20) continue;

        bool occlusionRejected = true;
        int sampleStepX = std::max( 1, (modelScreenMaxX - modelScreenMinX + 1) / 24 );
        for (int sx = modelScreenMinX; sx <= modelScreenMaxX; sx += sampleStepX)
        {
            if (modelNearestZ < (engineContext.zbuffer[ sx ] - kCullingEpsilon))
            {
                occlusionRejected = false;
                break;
            }
        }
        if (occlusionRejected && (modelScreenMaxX != modelScreenMinX))
        {
            if (modelNearestZ < (engineContext.zbuffer[ modelScreenMaxX ] - kCullingEpsilon))
            {
                occlusionRejected = false;
            }
        }
        if (occlusionRejected) continue;

        int renderedTrianglesForModel = 0;
        const int triangleBudget = g_perfLowMode ? 520 : std::numeric_limits<int>::max();
        const int triStride = std::max( 1, g_meshTriangleStride );
        const int rasterStep = std::max( 1, g_meshRasterStep );

        for (size_t ii = 0; ii + 2 < inst.model->indices.size(); ii += 3)
        {
            const int triIdx = int( ii / 3 );
            if (triStride > 1 && (triIdx % triStride) != 0) continue;
            if (renderedTrianglesForModel >= triangleBudget) break;

            const uint32_t i0 = inst.model->indices[ ii + 0 ];
            const uint32_t i1 = inst.model->indices[ ii + 1 ];
            const uint32_t i2 = inst.model->indices[ ii + 2 ];
            if (i0 >= projected.size() || i1 >= projected.size() || i2 >= projected.size()) continue;

            const CpuProjVert &a = projected[ i0 ];
            const CpuProjVert &b = projected[ i1 ];
            const CpuProjVert &c = projected[ i2 ];
            if (!a.valid || !b.valid || !c.valid) continue;
            if (a.z <= nearClip || b.z <= nearClip || c.z <= nearClip) continue;

            const float area = (b.sx - a.sx) * (c.sy - a.sy) - (b.sy - a.sy) * (c.sx - a.sx);
            if (std::fabs( area ) < 1e-5f) continue;

            int minX = std::max( 0, (int)std::floor( std::min( { a.sx, b.sx, c.sx } ) ) );
            int maxX = std::min( RENDER_W - 1, (int)std::ceil( std::max( { a.sx, b.sx, c.sx } ) ) );
            int minY = std::max( 0, (int)std::floor( std::min( { a.sy, b.sy, c.sy } ) ) );
            int maxY = std::min( RENDER_H - 1, (int)std::ceil( std::max( { a.sy, b.sy, c.sy } ) ) );

            if (minX > maxX || minY > maxY) continue;
            if ((maxX - minX) > (RENDER_W - 8) || (maxY - minY) > (RENDER_H - 8)) continue;

            if (g_perfLowMode)
            {
                const int triW = maxX - minX + 1;
                const int triH = maxY - minY + 1;
                if ((triW * triH) <= 2) continue;

                const float triMidZ = (a.z + b.z + c.z) * (1.0f / 3.0f);
                if (triMidZ > (g_worldModelRenderDistance * 0.55f) && (triIdx & 1))
                {
                    continue;
                }
            }

            minY = std::max( minY, yStartInclusive );
            maxY = std::min( maxY, yEndExclusive - 1 );
            if (minY > maxY) continue;

            glm::vec3 nrm = glm::cross( b.world - a.world, c.world - a.world );
            const float nLen = glm::length( nrm );
            if (nLen <= 1e-6f) continue;
            nrm /= nLen;
            const float lambert = std::clamp( 0.35f + 0.65f * std::fabs( glm::dot( nrm, lightDir ) ), 0.20f, 1.0f );

            const float triDepth = (a.z + b.z + c.z) * (1.0f / 3.0f);
            float distanceShade;
            if (engineContext.caveMode)
            {
                const float R = engineContext.lightRadius;
                const float t = std::clamp( 1.0f - std::pow( triDepth / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
                distanceShade = std::max( engineContext.caveAmbient, t );
            }
            else
            {
                distanceShade = 1.0f / (1.0f + engineContext.indoorShadeLinear * triDepth + engineContext.indoorShadeQuadratic * triDepth * triDepth);
                distanceShade = std::clamp( distanceShade, engineContext.indoorShadeMin, 1.0f );
            }

            glm::vec3 materialColor( 1.0f );
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleBaseColorFactor.size())
            {
                const glm::vec4 f = inst.model->triangleBaseColorFactor[ triIdx ];
                materialColor = glm::vec3( f.r, f.g, f.b );
            }

            int texIdx = -1;
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleTextureIndex.size())
            {
                texIdx = inst.model->triangleTextureIndex[ triIdx ];
            }

            const Image *tex = nullptr;
            if (texIdx >= 0 && texIdx < (int)inst.model->baseColorTextures.size())
            {
                const Image &candidate = inst.model->baseColorTextures[ texIdx ];
                if (candidate.width > 0 && candidate.height > 0)
                {
                    tex = &candidate;
                }
            }

            glm::vec3 vertexMul( 1.0f );
            if (!tex)
            {
                vertexMul = (a.color + b.color + c.color) * (1.0f / 3.0f);
            }

            const float lit = std::clamp( 0.16f + 0.90f * (lambert * distanceShade), 0.16f, 1.00f );
            const int shadeR256 = toFixed256( materialColor.r * vertexMul.r * lit * museumPowerMul );
            const int shadeG256 = toFixed256( materialColor.g * vertexMul.g * lit * museumPowerMul );
            const int shadeB256 = toFixed256( materialColor.b * vertexMul.b * lit * museumPowerMul );
            const Uint32 solidColor = rgb(
                Uint8( (255 * shadeR256) >> 8 ),
                Uint8( (255 * shadeG256) >> 8 ),
                Uint8( (255 * shadeB256) >> 8 ) );

            const float invZ0 = 1.0f / std::max( 0.0001f, a.z );
            const float invZ1 = 1.0f / std::max( 0.0001f, b.z );
            const float invZ2 = 1.0f / std::max( 0.0001f, c.z );

            const float invArea = 1.0f / area;

            // Edge stepping for E(p) = (v0.x - p.x)*(v1.y - p.y) - (v0.y - p.y)*(v1.x - p.x)
            const float w0dx = (b.sy - c.sy) * float( rasterStep );
            const float w0dy = (c.sx - b.sx) * float( rasterStep );
            const float w1dx = (c.sy - a.sy) * float( rasterStep );
            const float w1dy = (a.sx - c.sx) * float( rasterStep );
            const float w2dx = (a.sy - b.sy) * float( rasterStep );
            const float w2dy = (b.sx - a.sx) * float( rasterStep );

            const float invZdx = (w0dx * invZ0 + w1dx * invZ1 + w2dx * invZ2) * invArea;
            const float invZdy = (w0dy * invZ0 + w1dy * invZ1 + w2dy * invZ2) * invArea;

            const float sampleStartX = float( minX ) + 0.5f;
            const float sampleStartY = float( minY ) + 0.5f;

            const float w0RowInit =
                (b.sx - sampleStartX) * (c.sy - sampleStartY) -
                (b.sy - sampleStartY) * (c.sx - sampleStartX);
            const float w1RowInit =
                (c.sx - sampleStartX) * (a.sy - sampleStartY) -
                (c.sy - sampleStartY) * (a.sx - sampleStartX);
            const float w2RowInit =
                (a.sx - sampleStartX) * (b.sy - sampleStartY) -
                (a.sy - sampleStartY) * (b.sx - sampleStartX);

            float w0Row = w0RowInit;
            float w1Row = w1RowInit;
            float w2Row = w2RowInit;

            float invZRow = (w0Row * invZ0 + w1Row * invZ1 + w2Row * invZ2) * invArea;

            int uFixedRow = 0;
            int vFixedRow = 0;
            int duFixedX = 0;
            int dvFixedX = 0;
            int duFixedY = 0;
            int dvFixedY = 0;

            if (tex)
            {
                const float u0 = a.uv.x;
                const float u1 = b.uv.x;
                const float u2 = c.uv.x;
                const float v0 = a.uv.y;
                const float v1 = b.uv.y;
                const float v2 = c.uv.y;

                const float udx = (w0dx * u0 + w1dx * u1 + w2dx * u2) * invArea;
                const float udy = (w0dy * u0 + w1dy * u1 + w2dy * u2) * invArea;
                const float vdx = (w0dx * v0 + w1dx * v1 + w2dx * v2) * invArea;
                const float vdy = (w0dy * v0 + w1dy * v1 + w2dy * v2) * invArea;

                const float uStart = (w0Row * u0 + w1Row * u1 + w2Row * u2) * invArea;
                const float vStart = (w0Row * v0 + w1Row * v1 + w2Row * v2) * invArea;

                const float uScale = float( tex->width ) * 65536.0f;
                const float vScale = float( tex->height ) * 65536.0f;

                uFixedRow = int( uStart * uScale );
                vFixedRow = int( vStart * vScale );
                duFixedX = int( udx * uScale );
                dvFixedX = int( vdx * vScale );
                duFixedY = int( udy * uScale );
                dvFixedY = int( vdy * vScale );
            }

            const bool areaPositive = area > 0.0f;

            for (int y = minY; y <= maxY; y += rasterStep)
            {
                float w0 = w0Row;
                float w1 = w1Row;
                float w2 = w2Row;
                float invZ = invZRow;
                int uFixed = uFixedRow;
                int vFixed = vFixedRow;

                for (int x = minX; x <= maxX; x += rasterStep)
                {
                    const bool inside = areaPositive
                        ? (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                        : (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);

                    if (inside && invZ > wallInvDepthBuffer[ x ])
                    {
                        const int pix = y * RENDER_W + x;
                        if (invZ > meshInvDepthBuffer[ pix ])
                        {
                            if (tex)
                            {
                                const int tx = wrapTextureCoord( uFixed >> 16, tex->width );
                                const int ty = wrapTextureCoord( vFixed >> 16, tex->height );
                                const Uint32 tc = tex->sample( tx, ty );
                                const Uint8 r = Uint8( (int( (tc >> 16) & 255 ) * shadeR256) >> 8 );
                                const Uint8 g = Uint8( (int( (tc >> 8) & 255 ) * shadeG256) >> 8 );
                                const Uint8 bch = Uint8( (int( tc & 255 ) * shadeB256) >> 8 );
                                engineContext.backbuffer[ pix ] = rgb( r, g, bch );
                            }
                            else
                            {
                                engineContext.backbuffer[ pix ] = solidColor;
                            }

                            meshInvDepthBuffer[ pix ] = invZ;
                        }
                    }

                    w0 += w0dx;
                    w1 += w1dx;
                    w2 += w2dx;
                    invZ += invZdx;
                    uFixed += duFixedX;
                    vFixed += dvFixedX;
                }

                w0Row += w0dy;
                w1Row += w1dy;
                w2Row += w2dy;
                invZRow += invZdy;
                uFixedRow += duFixedY;
                vFixedRow += dvFixedY;
            }

            ++renderedTrianglesForModel;
        }
    }
}

static void rasterWorkerThreadMain( unsigned int workerIndex ) {
    uint64_t observedSerial = 0;

    while (true)
    {
        Engine *jobEngine = nullptr;
        std::vector<float> *jobMeshDepth = nullptr;
        const std::vector<float> *jobWallDepth = nullptr;
        float jobPitchOffset = 0.0f;
        unsigned int jobWorkerCount = 0;
        uint64_t localSerial = 0;

        {
            std::unique_lock<std::mutex> lock( g_rasterWorkMutex );
            g_rasterWorkCv.wait( lock, [&]() {
                return g_rasterPoolShutdown || (g_rasterJobAvailable && g_rasterJobSerial != observedSerial);
            } );

            if (g_rasterPoolShutdown)
            {
                return;
            }

            localSerial = g_rasterJobSerial;
            observedSerial = localSerial;
            jobEngine = g_rasterJobEngine;
            jobMeshDepth = g_rasterJobMeshDepth;
            jobWallDepth = g_rasterJobWallDepth;
            jobPitchOffset = g_rasterJobPitchOffset;
            jobWorkerCount = g_rasterJobWorkerCount;
        }

        if (workerIndex < jobWorkerCount && jobEngine && jobMeshDepth && jobWallDepth)
        {
            const int y0 = int( (workerIndex * RENDER_H) / jobWorkerCount );
            const int y1 = int( ((workerIndex + 1) * RENDER_H) / jobWorkerCount );
            renderWorldModelsRange( *jobEngine, *jobMeshDepth, *jobWallDepth, jobPitchOffset, y0, y1 );

            std::lock_guard<std::mutex> doneLock( g_rasterWorkMutex );
            if (g_rasterJobSerial == localSerial)
            {
                ++g_rasterJobCompleted;
                if (g_rasterJobCompleted >= g_rasterJobWorkerCount)
                {
                    g_rasterDoneCv.notify_one();
                }
            }
        }
    }
}

static void initRasterWorkerPool() {
    if (!g_rasterWorkers.empty()) return;
    if (g_detectedThreadCount <= 1u) return;

    g_rasterPoolShutdown = false;
    g_rasterJobAvailable = false;
    g_rasterJobSerial = 0;
    g_rasterJobWorkerCount = 0;
    g_rasterJobCompleted = 0;

    g_rasterWorkers.reserve( g_detectedThreadCount );
    for (unsigned int i = 0; i < g_detectedThreadCount; ++i)
    {
        g_rasterWorkers.emplace_back( rasterWorkerThreadMain, i );
    }
}

static void shutdownRasterWorkerPool() {
    {
        std::lock_guard<std::mutex> lock( g_rasterWorkMutex );
        g_rasterPoolShutdown = true;
    }
    g_rasterWorkCv.notify_all();

    for (auto &t : g_rasterWorkers)
    {
        if (t.joinable()) t.join();
    }
    g_rasterWorkers.clear();
}

static void dispatchRasterWorkers(
    Engine &engineContext,
    std::vector<float> &meshDepthBuffer,
    const std::vector<float> &wallInvDepthBuffer,
    float pitchOffset,
    unsigned int workerCount ) {
    workerCount = std::max( 1u, workerCount );

    {
        std::lock_guard<std::mutex> lock( g_rasterWorkMutex );
        g_rasterJobEngine = &engineContext;
        g_rasterJobMeshDepth = &meshDepthBuffer;
        g_rasterJobWallDepth = &wallInvDepthBuffer;
        g_rasterJobPitchOffset = pitchOffset;
        g_rasterJobWorkerCount = workerCount;
        g_rasterJobCompleted = 0;
        g_rasterJobAvailable = true;
        ++g_rasterJobSerial;
    }

    g_rasterWorkCv.notify_all();

    std::unique_lock<std::mutex> doneLock( g_rasterWorkMutex );
    g_rasterDoneCv.wait( doneLock, [&]() {
        return g_rasterJobCompleted >= g_rasterJobWorkerCount;
    } );
}

static void renderWorldModels( Engine &engineContext, std::vector<float> &meshDepthBuffer, float pitchOffset) {
    if (g_worldModels.empty()) return;

    std::vector<float> wallInvDepthBuffer;
    wallInvDepthBuffer.resize( RENDER_W, 0.0f );
    for (int x = 0; x < RENDER_W; ++x)
    {
        const float wz = engineContext.zbuffer[ x ];
        wallInvDepthBuffer[ x ] = (wz > 0.0001f) ? (1.0f / wz) : std::numeric_limits<float>::infinity();
    }

    const int fullYStart = 0;
    const int fullYEnd = RENDER_H;
    const unsigned int threadCount = std::max( 1u, g_detectedThreadCount );
    const bool useMultithreading = g_multithreadingEnabled && threadCount > 1 && g_worldModels.size() >= 4;

    if (!useMultithreading)
    {
        renderWorldModelsRange( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, fullYStart, fullYEnd );
        return;
    }

    const unsigned int maxUsefulThreadsByRows = std::max( 1u, (unsigned int)(RENDER_H / 96) );
    const unsigned int workerCount = std::max( 1u, std::min( threadCount, maxUsefulThreadsByRows ) );

    if (workerCount <= 1u || g_rasterWorkers.empty())
    {
        renderWorldModelsRange( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, fullYStart, fullYEnd );
        return;
    }

    dispatchRasterWorkers( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, workerCount );
}

static void renderWorldModelsGpu( Engine &engineContext, float pitchOffset ) {
    if (!isGpuModelRenderingEnabled()) return;
    if (!engineContext.renderer || g_worldModels.empty()) return;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );
    const float projScaleY = (RENDER_W * 0.5f);
    const float horizon = (RENDER_H * 0.5f) + pitchOffset;
    const float camHeight = 0.52f;
    const float nearClip = 0.18f;
    const float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);

    struct GpuVert { float sx = 0, sy = 0, z = -1; glm::vec3 world{0.0f}; glm::vec3 vcolor{1.0f}; glm::vec2 uv{0.0f}; bool valid = false; };
    struct DrawTri { SDL_Vertex a{}, b{}, c{}; float z = 0.0f; SDL_Texture* texture = nullptr; };

    std::vector<DrawTri> tris;
    tris.reserve( 4096 );

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.model || inst.model->indices.size() < 3) continue;
        if (!shouldGpuRenderModel( engineContext, inst )) continue;

        if (inst.model->hwTextures.size() != inst.model->baseColorTextures.size()) {
            inst.model->hwTextures.resize(inst.model->baseColorTextures.size(), nullptr);
            for (size_t t = 0; t < inst.model->baseColorTextures.size(); ++t) {
                const auto& img = inst.model->baseColorTextures[t];
                if (img.width > 0 && img.height > 0) {
                    SDL_Texture* tex = SDL_CreateTexture(engineContext.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, img.width, img.height);
                    if (tex)
                    {
                        SDL_UpdateTexture(tex, nullptr, img.pixels.data(), img.width * 4);
                        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
                    }
                    inst.model->hwTextures[t] = tex;
                }
            }
        }

        const glm::vec3 modelHalfExtents = glm::max( (inst.model->boundsMax - inst.model->boundsMin) * 0.5f, glm::vec3( 0.0001f ) ) * inst.scale;
        const float modelRadius = std::max( 0.05f, glm::length( modelHalfExtents ) );

        const float centerDx = inst.x - engineContext.positionX;
        const float centerDy = inst.y - engineContext.positionY;
        const float centerTz = invDet * (-engineContext.planeY * centerDx + engineContext.planeX * centerDy);
        if ((centerTz + modelRadius) <= nearClip) continue;
        if ((centerTz - modelRadius) > g_worldModelRenderDistance) continue;

        std::vector<GpuVert> projected;
        projected.resize( inst.model->vertices.size() );
        std::vector<glm::vec3> transformed;
        transformed.resize( inst.model->vertices.size(), glm::vec3( 0.0f ) );

        const float timeSeconds = SDL_GetTicks() * 0.001f;
        const float yawNow = inst.yaw + (inst.spinYaw ? (inst.spinSpeed * timeSeconds) : 0.0f);
        const glm::quat qYaw = glm::angleAxis( yawNow, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        const glm::quat qPitch = glm::angleAxis( inst.pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        const glm::quat qRoll = glm::angleAxis( inst.roll, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        const glm::quat q = qYaw * qPitch * qRoll;
        const glm::vec3 pivot(
            (inst.model->boundsMin.x + inst.model->boundsMax.x) * 0.5f,
            inst.model->boundsMin.y,
            (inst.model->boundsMin.z + inst.model->boundsMax.z) * 0.5f );

        float modelMinY = std::numeric_limits<float>::max();
        for (size_t i = 0; i < inst.model->vertices.size(); ++i)
        {
            const glm::vec3 local = (inst.model->vertices[ i ] - pivot) * inst.scale;
            transformed[ i ] = q * local;
            modelMinY = std::min( modelMinY, transformed[ i ].y );
        }
        if (!std::isfinite( modelMinY )) modelMinY = 0.0f;

        for (size_t i = 0; i < inst.model->vertices.size(); ++i)
        {
            const glm::vec3 r = transformed[ i ];
            const float wx = inst.x + r.x;
            const float wy = (r.y - modelMinY) + inst.heightOffset;
            const float wz = inst.y + r.z;

            const float dx = wx - engineContext.positionX;
            const float dy = wz - engineContext.positionY;
            const float tx = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            const float tz = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (tz <= nearClip) continue;

            projected[ i ].sx = (RENDER_W * 0.5f) * (1.0f + (tx / tz));
            projected[ i ].sy = horizon - ((wy - camHeight) * projScaleY / tz);
            projected[ i ].z = tz;
            projected[ i ].world = glm::vec3( wx, wy, wz );
            if (i < inst.model->colors.size()) projected[ i ].vcolor = inst.model->colors[ i ];
            if (i < inst.model->uvs.size()) projected[ i ].uv = inst.model->uvs[ i ];
            projected[ i ].valid = true;
        }

        const int triStride = std::max( 1, g_meshTriangleStride );
        int triBudget = g_perfLowMode ? 650 : std::numeric_limits<int>::max();

        for (size_t i = 0; i + 2 < inst.model->indices.size(); i += 3)
        {
            if (triBudget <= 0) break;
            const int triIdx = int( i / 3 );
            if (triStride > 1 && (triIdx % triStride) != 0) continue;

            const uint32_t i0 = inst.model->indices[ i + 0 ];
            const uint32_t i1 = inst.model->indices[ i + 1 ];
            const uint32_t i2 = inst.model->indices[ i + 2 ];
            if (i0 >= projected.size() || i1 >= projected.size() || i2 >= projected.size()) continue;

            const GpuVert &a = projected[ i0 ];
            const GpuVert &b = projected[ i1 ];
            const GpuVert &c = projected[ i2 ];
            if (!a.valid || !b.valid || !c.valid) continue;

            const float area = (b.sx - a.sx) * (c.sy - a.sy) - (b.sy - a.sy) * (c.sx - a.sx);
            if (std::fabs( area ) < 0.01f) continue;
            if (area >= -0.01f) continue;

            const int minX = std::max( 0, (int)std::floor( std::min( { a.sx, b.sx, c.sx } ) ) );
            const int maxX = std::min( RENDER_W - 1, (int)std::ceil( std::max( { a.sx, b.sx, c.sx } ) ) );
            const int minY = std::max( 0, (int)std::floor( std::min( { a.sy, b.sy, c.sy } ) ) );
            const int maxY = std::min( RENDER_H - 1, (int)std::ceil( std::max( { a.sy, b.sy, c.sy } ) ) );
            if (minX > maxX || minY > maxY) continue;

            const float triMidZ = (a.z + b.z + c.z) * (1.0f / 3.0f);
            const int sxA = std::clamp( (int)a.sx, 0, RENDER_W - 1 );
            const int sxB = std::clamp( (int)b.sx, 0, RENDER_W - 1 );
            const int sxC = std::clamp( (int)c.sx, 0, RENDER_W - 1 );
            if (a.z >= engineContext.zbuffer[ sxA ] && b.z >= engineContext.zbuffer[ sxB ] && c.z >= engineContext.zbuffer[ sxC ]) {
                continue;
            }

            const glm::vec3 nrm = glm::normalize( glm::cross( b.world - a.world, c.world - a.world ) );
            const float lambert = std::clamp( 0.25f + 0.75f * std::fabs( glm::dot( nrm, glm::normalize( glm::vec3( -0.35f, 0.85f, -0.40f ) ) ) ), 0.15f, 1.0f );
            float shade = 1.0f;
            if (engineContext.caveMode)
            {
                float R = engineContext.lightRadius;
                float t = std::clamp( 1.0f - std::pow( triMidZ / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
                shade = std::max( engineContext.caveAmbient, t );
            }
            else
            {
                shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * triMidZ + engineContext.indoorShadeQuadratic * triMidZ * triMidZ);
                shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
            }
            glm::vec3 baseColor( 1.0f );
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleBaseColorFactor.size())
            {
                const glm::vec4 f = inst.model->triangleBaseColorFactor[ triIdx ];
                baseColor = glm::vec3( f.r, f.g, f.b );
            }

            int texIdx = -1;
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleTextureIndex.size())
            {
                texIdx = inst.model->triangleTextureIndex[ triIdx ];
            }

            const glm::vec3 vAvg = (a.vcolor + b.vcolor + c.vcolor) * (1.0f / 3.0f);
            const float tr = float( (inst.tint >> 16) & 255 ) / 255.0f;
            const float tg = float( (inst.tint >> 8) & 255 ) / 255.0f;
            const float tb = float( inst.tint & 255 ) / 255.0f;

            const glm::vec3 rgb = glm::clamp(
                baseColor * vAvg * glm::vec3( tr, tg, tb ) *
                std::clamp( lambert * shade * engineContext.ambianceMul * g_horrorLightingMul * museumPowerMul, 0.10f, 1.0f ),
                glm::vec3( 0.0f ), glm::vec3( 1.0f ) );
            const SDL_FColor col{ rgb.r, rgb.g, rgb.b, 1.0f };

            DrawTri out{};
            out.z = triMidZ;
            out.a.position = SDL_FPoint{ a.sx * float( WIN_SCALE ), a.sy * float( WIN_SCALE ) };
            out.b.position = SDL_FPoint{ b.sx * float( WIN_SCALE ), b.sy * float( WIN_SCALE ) };
            out.c.position = SDL_FPoint{ c.sx * float( WIN_SCALE ), c.sy * float( WIN_SCALE ) };
            out.a.tex_coord = SDL_FPoint{ a.uv.x, a.uv.y };
            out.b.tex_coord = SDL_FPoint{ b.uv.x, b.uv.y };
            out.c.tex_coord = SDL_FPoint{ c.uv.x, c.uv.y };
            out.a.color = col; out.b.color = col; out.c.color = col;
            out.texture = (texIdx >= 0 && texIdx < (int)inst.model->hwTextures.size()) ? inst.model->hwTextures[ texIdx ] : nullptr;

            tris.push_back( out );

            --triBudget;
        }
    }

    if (!tris.empty())
    {
        std::sort( tris.begin(), tris.end(), []( const DrawTri &lhs, const DrawTri &rhs ) {
            return lhs.z > rhs.z; // draw far-to-near to emulate opaque depth ordering
            } );

        SDL_Texture* currentTex = tris[ 0 ].texture;
        std::vector<SDL_Vertex> batch;
        batch.reserve( 768 );

        auto flushBatch = [&]() {
            if (batch.empty()) return;
            SDL_RenderGeometry( engineContext.renderer, currentTex, batch.data(), (int)batch.size(), nullptr, 0 );
            batch.clear();
        };

        for (const DrawTri &t : tris)
        {
            if (t.texture != currentTex)
            {
                flushBatch();
                currentTex = t.texture;
            }

            batch.push_back( t.a );
            batch.push_back( t.b );
            batch.push_back( t.c );
        }

        flushBatch();
    }
}

static void render( Engine &engineContext, float dt ) {
    (void)dt;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );

    const float shotFx01 = std::clamp( g_revolverRecoilTimer / std::max( 0.001f, kRevolverRecoilDuration ), 0.0f, 1.0f );
    const float shotShakeWave = std::pow( shotFx01, 0.56f );
    const float shakePhase = SDL_GetTicks() * 0.001f;
    const float shotPitchShake =
        (std::sin( shakePhase * 92.0f ) * 0.78f + std::cos( shakePhase * 141.0f ) * 0.42f) *
        kRevolverScreenShakeY * shotShakeWave;
    const float recoilKickPitch = shotFx01 * 6.0f;
    const float effectivePitchOffset = engineContext.pitchOffset + shotPitchShake + recoilKickPitch;
    g_lastEffectivePitchOffset = effectivePitchOffset;

    auto luma = []( Uint32 c ) -> float {
        float r = float( (c >> 16) & 255 ), g = float( (c >> 8) & 255 ), b = float( c & 255 );
        return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
        };

  
    auto mulFromOverlay = [&]( Uint32 oc, float strength, float minMul, float maxMul, float gamma = 1.0f ) -> float {
        float L = std::pow( std::clamp( luma( oc ), 0.0f, 1.0f ), gamma );
        float m = 1.0f - strength * (1.0f - L);               // dark pixels -> lower multiplier
        return std::clamp( m, minMul, maxMul );
        };

    // Apply brightness multiplier to a packed ARGB8888 color (no hue shift)
    auto applyMul = [&]( Uint32 base, float m ) -> Uint32 {
        float rf = float( (base >> 16) & 255 ) * m;
        float gf = float( (base >> 8) & 255 ) * m;
        float bf = float( base & 255 ) * m;
        Uint8 r = Uint8( std::clamp( rf, 0.0f, 255.0f ) );
        Uint8 g = Uint8( std::clamp( gf, 0.0f, 255.0f ) );
        Uint8 b = Uint8( std::clamp( bf, 0.0f, 255.0f ) );
        return rgb( r, g, b );
        };

    auto shadeCol = []( Uint32 c, float s ) -> Uint32 {
        s = std::clamp( s, 0.0f, 1.0f );
        Uint8 r = Uint8( ((c >> 16) & 255) * s );
        Uint8 g = Uint8( ((c >> 8) & 255) * s );
        Uint8 b = Uint8( (c & 255) * s );
        return rgb( r, g, b );
        };

    auto applyAmbience = [&]( Uint32 c, float shade ) -> Uint32 {
        Uint32 shaded = shadeCol( c, shade );
        float tr = float( (engineContext.ambianceTint >> 16) & 255 ) / 255.0f;
        float tg = float( (engineContext.ambianceTint >> 8) & 255 ) / 255.0f;
        float tb = float( engineContext.ambianceTint & 255 ) / 255.0f;
        const float horrorMul = std::clamp( g_horrorLightingMul, 0.35f, 1.0f );
        Uint8 r = Uint8( std::clamp( float( (shaded >> 16) & 255 ) * tr * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        Uint8 g = Uint8( std::clamp( float( (shaded >> 8) & 255 ) * tg * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        Uint8 b = Uint8( std::clamp( float( shaded & 255 ) * tb * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        return rgb( r, g, b );
        };

    auto caveLight = [&]( float dist ) -> float {
        if (!engineContext.caveMode) return 1.0f;
        float R = engineContext.lightRadius;
        float t = std::clamp( 1.0f - std::pow( dist / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
        return std::max( engineContext.caveAmbient, t );
        };

    updateHeldRevolverModel( engineContext );

    const int half = RENDER_H / 2;
    engineContext.zbuffer.assign( RENDER_W, 1e9f );

    static int clipTop[ RENDER_W ];
    static int clipBot[ RENDER_W ];
    for (int i = 0; i < RENDER_W; ++i)
    {
        clipTop[ i ] = RENDER_H;
        clipBot[ i ] = -1;
    }

	// Walls (raycasted)
    for (int x = 0; x < RENDER_W; ++x)
    {

        // Build ray
        float camX = 2.0f * x / float( RENDER_W ) - 1.0f;
        float rayDirX = engineContext.directionX + engineContext.planeX * camX;
        float rayDirY = engineContext.directionY + engineContext.planeY * camX;

        int mapX = int( engineContext.positionX );
        int mapY = int( engineContext.positionY );

        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::fabs( 1.0f / rayDirX );
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::fabs( 1.0f / rayDirY );
        int stepX = 0, stepY = 0, side = 0;

        if (rayDirX < 0)
        {
            stepX = -1; 
            sideDistX = (engineContext.positionX - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1; 
            sideDistX = (mapX + 1.0f - engineContext.positionX) * deltaDistX;
        }
        if (rayDirY < 0)
        {
            stepY = -1; 
            sideDistY = (engineContext.positionY - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1; 
            sideDistY = (mapY + 1.0f - engineContext.positionY) * deltaDistY;
        }

        // DDA
        int hitTile = 0;
        while (!hitTile)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX; 
                mapX += stepX; 
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY; 
                mapY += stepY; 
                side = 1;
            }

            const float rayTravel = std::min( sideDistX, sideDistY );
            if (rayTravel > g_wallRenderDistance)
            {
                break;
            }

            if (mapX < 0 || mapY < 0 || mapX >= engineContext.map.width || mapY >= engineContext.map.height) break;
            int tile = engineContext.map.tiles[ mapY * engineContext.map.width + mapX ];
            if (tile > 0) hitTile = tile;
        }
        if (!hitTile) continue;

        // Perpendicular distance
        float perpWallDist = (side == 0)
            ? ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX)
            : ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
        perpWallDist = std::max( std::fabs( perpWallDist ), 0.05f );
        if (perpWallDist > g_wallRenderDistance) continue;

        // Column geometry
        int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );

        int bob = half + (int)effectivePitchOffset;
        int drawStart = std::max( 0, -lineH / 2 + bob );
        int drawEnd = std::min( RENDER_H - 1, lineH / 2 + bob );
        clipTop[ x ] = std::min( clipTop[ x ], drawStart );
        clipBot[ x ] = std::max( clipBot[ x ], drawEnd );
        // Wall X coordinate (for texture)
        float wallX = (side == 0)
            ? (engineContext.positionY + perpWallDist * rayDirY)
            : (engineContext.positionX + perpWallDist * rayDirX);
        wallX -= std::floor( wallX );

        float wallShade = 1.0f;
        if (engineContext.caveMode) {
            wallShade = caveLight(perpWallDist);
        }
        else {
            wallShade = 1.0f / (1.0f + engineContext.indoorShadeLinear * perpWallDist + engineContext.indoorShadeQuadratic * perpWallDist * perpWallDist);
            wallShade = std::clamp(wallShade, engineContext.indoorShadeMin, 1.0f);
        }
        if (side == 1) wallShade *= 0.75f; // Apply side shading for depth

        // Texture selection
        const Image &wallTexture = (hitTile == 2) ? engineContext.doorTexture : engineContext.wallTex;

        drawTexturedColumn(engineContext, wallTexture, x, drawStart, drawEnd, perpWallDist, wallX, side);

        if (hitTile == 1)
        {
            if ((engineContext.currentLevel == Levels::MUSEUM || engineContext.currentLevel == Levels::MUSEUM_UPPER) && !(g_perfLowMode && (x & 1))) {
                for (size_t artIndex = 0; artIndex < engineContext.artworks.size(); ++artIndex)
                {
                    const auto& art = engineContext.artworks[artIndex];
                    if (!art.onWall) continue;


                    if (art.wx != mapX || art.wy != mapY || art.side != side) continue;

                    
                    bool visible = true;

                    if (side == 0)
                    { // Vertical Wall (X-Axis)
                        float frac = art.x - std::floor( art.x ); // e.g., 3.1 -> 0.1

                        // Ray moving Right (>0) hits West Face. Ray moving Left (<0) hits East Face.
                        bool hittingWestFace = (rayDirX > 0);

                        if (frac < 0.45f && !hittingWestFace) visible = false; // Art is on West, but we hit East
                        if (frac > 0.55f && hittingWestFace)  visible = false; // Art is on East, but we hit West
                    }
                    else
                    { // Horizontal Wall (Y-Axis)
                        float frac = art.y - std::floor( art.y );

                        // Ray moving Down (>0) hits North Face. Ray moving Up (<0) hits South Face.
                        bool hittingNorthFace = (rayDirY > 0);

                        if (frac < 0.45f && !hittingNorthFace) visible = false; // Art is on North, but we hit South
                        if (frac > 0.55f && hittingNorthFace)  visible = false; // Art is on South, but we hit North
                    }

                    if (!visible) continue;
                    

                    float u0 = std::clamp(art.uCenter - art.uWidth * 0.5f, 0.0f, 1.0f);
                    float u1 = std::clamp(art.uCenter + art.uWidth * 0.5f, 0.0f, 1.0f);
                    if (wallX < u0 || wallX > u1) continue;

                    const Image& texture = engineContext.artImages[artIndex];

                    // Frame/mat proportions
                    const float FRAME_U = 0.08f, FRAME_V = 0.08f;
                    const float MAT_U = 0.03f, MAT_V = 0.04f;

                    const Uint32 goldLight = rgb(235, 200, 80);
                    const Uint32 goldMid = rgb(212, 175, 55);
                    const Uint32 goldDark = rgb(160, 130, 40);
                    const Uint32 matCol = rgb(235, 235, 220);

                    float uLocal = (wallX - u0) / std::max(0.0001f, (u1 - u0));

                    float horizon = (RENDER_H / 2.0f) + engineContext.pitchOffset;

                    int bandH = std::max(1, int(lineH * art.vHeight));
                    int bandCenter = int(horizon + (art.vCenter - 0.5f) * lineH);
                    int bandStart = bandCenter - bandH / 2;
                    int bandEnd = bandStart + bandH - 1;

                    int drawStart = std::clamp(bandStart, 0, RENDER_H - 1);
                    int drawEnd = std::clamp(bandEnd, 0, RENDER_H - 1);

                    float uLeftFrameEdge = FRAME_U;
                    float uRightFrameEdge = 1.0f - FRAME_U;
                    float uLeftMatEdge = FRAME_U + MAT_U;
                    float uRightMatEdge = 1.0f - (FRAME_U + MAT_U);

                    for (int y = drawStart; y <= drawEnd; ++y)
                    {
                        float vLocal = (y - bandStart) / float(std::max(1, bandH - 1));
                        float vTopFrameEdge = FRAME_V;
                        float vBottomFrameEdge = 1.0f - FRAME_V;
                        float vTopMatEdge = FRAME_V + MAT_V;
                        float vBottomMatEdge = 1.0f - (FRAME_V + MAT_V);

                        Uint32 color;

                        bool inFrame =
                            (uLocal < uLeftFrameEdge) || (uLocal > uRightFrameEdge) ||
                            (vLocal < vTopFrameEdge) || (vLocal > vBottomFrameEdge);

                        if (inFrame)
                        {
                            bool topOrLeft = (vLocal < vTopFrameEdge + 0.02f) || (uLocal < uLeftFrameEdge + 0.02f);
                            bool bottomOrRight = (vLocal > vBottomFrameEdge - 0.02f) || (uLocal > uRightFrameEdge - 0.02f);
                            color = goldMid;
                            if (topOrLeft)
                            {
                                color = goldLight;
                            }
                            else if (bottomOrRight)
                            {
                                color = goldDark;
                            }
                        }
                        else
                        {
                            bool inMat =
                                (uLocal < uLeftMatEdge) || (uLocal > uRightMatEdge) ||
                                (vLocal < vTopMatEdge) || (vLocal > vBottomMatEdge);

                            if (inMat)
                            {
                                color = matCol;
                            }
                            else
                            {
                                float innerU0 = uLeftMatEdge, innerU1 = uRightMatEdge;
                                float innerV0 = vTopMatEdge, innerV1 = vBottomMatEdge;
                                float un = (uLocal - innerU0) / std::max(0.0001f, (innerU1 - innerU0));
                                float vn = (vLocal - innerV0) / std::max(0.0001f, (innerV1 - innerV0));
                                int texX = std::clamp(int(un * (texture.width - 1)), 0, texture.width - 1);
                                int texY = std::clamp(int(vn * (texture.height - 1)), 0, texture.height - 1);
                                color = texture.sample(texX, texY);

                                // magenta transparent -> mat
                                if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255)
                                    color = matCol;
                            }
                        }
                        putPix(engineContext, x, y, color);
                    }
                }
            }
            if (engineContext.currentLevel == Levels::MUSEUM && g_stairWallOverlayReady)
            {
                bool stairWallTile = (side == 0) && (mapX == 22) && (mapY == 9);
                if (stairWallTile)
                {
                    float u0 = 0.0f;
                    float u1 = 1.0f;
                    if (wallX >= u0 && wallX <= u1)
                    {
                        int bandStart = drawStart;
                        int bandEnd = drawEnd;
                        int bandH = std::max( 1, bandEnd - bandStart + 1 );

                        float un = (wallX - u0) / std::max( 0.0001f, (u1 - u0) );
                        int texX = std::clamp( int( un * (g_stairWallOverlay.width - 1) ), 0, g_stairWallOverlay.width - 1 );

                        for (int y = bandStart; y <= bandEnd; ++y)
                        {
                            float vn = (y - bandStart) / float( std::max( 1, bandH - 1 ) );
                            int texY = std::clamp( int( vn * (g_stairWallOverlay.height - 1) ), 0, g_stairWallOverlay.height - 1 );
                            Uint32 c = g_stairWallOverlay.sample( texX, texY );
                            if (((c >> 16) & 255) == 255 && ((c >> 8) & 255) == 0 && (c & 255) == 255) continue;
                            putPix( engineContext, x, y, c );
                        }
                    }
                }
            }
            else if (engineContext.currentLevel == Levels::CAVE) {

            }
        }

        // Fill zbuffer for sprites/floor/ceiling occlusion
        engineContext.zbuffer[ x ] = perpWallDist;
    }

    // Floor and ceiling 
    float rayDirX0 = engineContext.directionX - engineContext.planeX;
    float rayDirY0 = engineContext.directionY - engineContext.planeY;
    float rayDirX1 = engineContext.directionX + engineContext.planeX;
    float rayDirY1 = engineContext.directionY + engineContext.planeY;

    const float posZ = 0.5f * RENDER_H;
    int bob = half + (int)effectivePitchOffset;

    for (int y = 0; y < RENDER_H; ++y)
    {
        const int prop = y - bob;
        if (prop == 0) continue;

        float rowDist = std::fabs( posZ / float( prop ) );

        // Step across row
        float stepX = rowDist * (rayDirX1 - rayDirX0) / float( RENDER_W );
        float stepY = rowDist * (rayDirY1 - rayDirY0) / float( RENDER_W );
        float worldX = engineContext.positionX + rowDist * rayDirX0;
        float worldY = engineContext.positionY + rowDist * rayDirY0;

        for (int x = 0; x < RENDER_W; ++x)
        {
            float fx = worldX - std::floor( worldX );
            float fy = worldY - std::floor( worldY );
            if (y >= clipTop[ x ] && y <= clipBot[ x ])
            {
                worldX += stepX;
                worldY += stepY;
                continue; // don't overwrite walls
            }

            if (y >= half)
            {
                if (engineContext.hasFloor)
                {
                    int tx = int( fx * engineContext.floorTex.width );
                    int ty = int( fy * engineContext.floorTex.height );
                    Uint32 color = engineContext.floorTex.sample( tx, ty );

                    float m = 1.0f;

                    if (engineContext.hasFloorStains && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayStains.width ) % engineContext.floorOverlayStains.width;
                        int oy = int( fy * engineContext.floorOverlayStains.height ) % engineContext.floorOverlayStains.height;
                        Uint32 oc = engineContext.floorOverlayStains.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.45f, /*min*/0.80f, /*max*/1.03f, /*gamma*/1.2f );
                    }
                    if (engineContext.hasFloorCracks && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayCracks.width ) % engineContext.floorOverlayCracks.width;
                        int oy = int( fy * engineContext.floorOverlayCracks.height ) % engineContext.floorOverlayCracks.height;
                        Uint32 oc = engineContext.floorOverlayCracks.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.85f, /*min*/0.55f, /*max*/1.00f, /*gamma*/1.6f );
                    }
                    if (engineContext.hasFloorPuddles && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayPuddles.width ) % engineContext.floorOverlayPuddles.width;
                        int oy = int( fy * engineContext.floorOverlayPuddles.height ) % engineContext.floorOverlayPuddles.height;
                        Uint32 oc = engineContext.floorOverlayPuddles.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.60f, /*min*/0.70f, /*max*/1.02f, /*gamma*/1.1f );
                    }

                    color = applyMul( color, m );

                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                        shade *= caveLight( rowDist );
                    }
                    else
                    {
                        // Museum Mode: Match the wall formula for consistency
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * rowDist + engineContext.indoorShadeQuadratic * rowDist * rowDist);
                        shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                    }
                    putPix( engineContext, x, y, applyAmbience( color, shade ) );

                    if (!g_perfLowMode && rowDist < engineContext.zbuffer[ x ] && !engineContext.quadBuckets.empty())
                    {
                        if (shade >= 0.06f) // skip work when very dark
                        {
                            int txTile = (int)std::floor( worldX );
                            int tyTile = (int)std::floor( worldY );
                            if ((unsigned)txTile < (unsigned)engineContext.map.width && (unsigned)tyTile < (unsigned)engineContext.map.height)
                            {
                                const auto &bucket = engineContext.quadBuckets[ tyTile * engineContext.map.width + txTile ];
                                for (int qi : bucket)
                                {
                                    const auto &q = engineContext.quads[ qi ];
                                    float u, v;
                                    if (!quadprop_local_uv( q, worldX, worldY, u, v )) continue;

                                    Uint32 dc = sample_bilinear_uv_keyed( q.texture, u, v );
                                    // magenta keyed; ignore transparent
                                    if (((dc >> 16) & 255) == 255 && ((dc >> 8) & 255) == 0 && (dc & 255) == 255) continue;

                                    // Treat quad as neutral detail: compute multiplier from its luminance
                                    float mul = mulFromOverlay( dc, /*strength*/1.00f, /*min*/0.55f, /*max*/1.05f, /*gamma*/1.4f );
                                    // Incorporate decal AO & cave light (as darkening influence)
                                    float ao = std::clamp( q.AOMultiplier, 0.5f, 1.0f );
                                    float l = caveLight( rowDist );
                                    float finalMul = std::clamp( mul * (0.9f + 0.1f * ao) * l, 0.0f, 1.05f );

                                    // Multiply the pixel already written in backbuffer
                                    Uint32 under = engineContext.backbuffer[ y * RENDER_W + x ];
                                    putPix( engineContext, x, y, applyMul( under, finalMul ) );
                                }
                            }
                        }
                    }
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 12, 12, 14 ) );
                }
            }
            else
            {
                if (y >= clipTop[ x ] && y <= clipBot[ x ])
                {
                    worldX += stepX;
                    worldY += stepY;
                    continue; // don't overwrite walls
                }

                // Ceiling
                if (engineContext.hasCeiling)
                {
                    int tx = int( fx * engineContext.ceilTex.width );
                    int ty = int( fy * engineContext.ceilTex.height );
                    Uint32 color = engineContext.ceilTex.sample( tx, ty );
                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                        shade *= caveLight( rowDist );
                    }
                    else
                    {
                        // Museum Mode: Match the wall formula for consistency
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * rowDist + engineContext.indoorShadeQuadratic * rowDist * rowDist);
                        shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                    }

                    putPix( engineContext, x, y, applyAmbience( color, shade ) );
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 30, 30, 38 ) );
                }
            }

            worldX += stepX;
            worldY += stepY;
        }
    }

    if (engineContext.benches3D.size() > 0)
    {
        for (const auto &box : engineContext.benches3D)
        {
            render_box_top( engineContext, box, box.sideTexure );
            render_box( engineContext, box );
            //render_legs( engineContext, box );
        }
    }

    if (!isGpuModelRenderingEnabled())
    {
        static std::vector<float> meshDepthBuffer;
        meshDepthBuffer.assign( RENDER_W * RENDER_H, 0.0f );
        renderWorldModels( engineContext, meshDepthBuffer, effectivePitchOffset );
    }
    else
    {
        // In Smart mode the CPU pass still handles the subset not offloaded to GPU.
        if (config::gpuRenderMode == 1)
        {
            static std::vector<float> meshDepthBuffer;
            meshDepthBuffer.assign( RENDER_W * RENDER_H, 0.0f );
            renderWorldModels( engineContext, meshDepthBuffer, effectivePitchOffset );
        }
    }



	// Props (billboarded)
    for (size_t i = 0; i < engineContext.props.size(); ++i)
    {
        const auto &prop = engineContext.props[ i ];
        if (prop.scale <= 0.0f) continue;
        const auto &texture = engineContext.propImages[ prop.textureID ];

        // Camera space
        float dx = prop.x - engineContext.positionX, dy = prop.y - engineContext.positionY;
        float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
        float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
        float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
        if (transY <= 0) continue;

        int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
        float baseH = (RENDER_H / transY);
        int spriteH = std::max( 1, int( std::fabs( baseH * prop.scale ) ) );
        int spriteW = spriteH;
        int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f );

        int y0 = bottomY - spriteH;
        int y1 = bottomY - 1;
        int x0 = -spriteW / 2 + spriteScreenX;
        int x1 = spriteW / 2 + spriteScreenX - 1;

        int cy0 = std::max( 0, y0 );
        int cy1 = std::min( RENDER_H - 1, y1 );
        int cx0 = std::max( 0, x0 );
        int cx1 = std::min( RENDER_W - 1, x1 );
        if (cy0 > cy1 || cx0 > cx1) continue;

        float invSpriteH = 1.0f / std::max( 1, spriteH );
        float invSpriteW = 1.0f / std::max( 1, spriteW );

        for (int sx = cx0; sx <= cx1; ++sx)
        {
            if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;

            float u = float( sx - x0 ) * invSpriteW;
            int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );

            int yStep = g_perfLowMode ? 2 : 1;
            for (int sy = cy0; sy <= cy1; sy += yStep)
            {
                float v = float( sy - y0 ) * invSpriteH;
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!isNearMagenta( color, 120 ))
                {
                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = caveLight( transY );
                    }
                    else
                    {
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * transY + engineContext.indoorShadeQuadratic * transY * transY);
                        shade = std::clamp( shade, 0.08f, 1.0f );
                    }
                    Uint32 shaded = applyAmbience( color, shade );
                    putPix( engineContext, sx, sy, shaded );
                    if (yStep == 2 && sy + 1 <= cy1)
                    {
                        putPix( engineContext, sx, sy + 1, shaded );
                    }
                }
            }
        }
    }

    if (!engineContext.columns.empty())
    {
        for (auto &col : engineContext.columns)
        {
            float dx = col.x - engineContext.positionX;
            float dy = col.y - engineContext.positionY;
            col.distance = dx * dx + dy * dy;
        }

        std::sort( engineContext.columns.begin(), engineContext.columns.end(), []( const ColumnProp &a, const ColumnProp &b ) {
            return a.distance > b.distance;
            } );

        for (const auto &col : engineContext.columns)
        {
            auto setIt = engineContext.columnSpriteSets.find( col.setName );
            if (setIt == engineContext.columnSpriteSets.end() || setIt->second.numViews == 0) continue;

            const SpriteSet &spriteSet = setIt->second;
            const int numViews = spriteSet.numViews;
            float toPlayerX = engineContext.positionX - col.x;
            float toPlayerY = engineContext.positionY - col.y;
            float rel = std::atan2( toPlayerY, toPlayerX );
            while (rel < 0.0f) rel += 2.0f * 3.14159265f;
            while (rel >= 2.0f * 3.14159265f) rel -= 2.0f * 3.14159265f;
            float slice = (2.0f * 3.14159265f) / numViews;
            int viewIndex = int( (rel + slice * 0.5f) / slice ) % numViews;
            const Image &texture = spriteSet.views[ viewIndex ];

            float dx = col.x - engineContext.positionX;
            float dy = col.y - engineContext.positionY;
            float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
            float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (transY <= 0) continue;

            int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
            float baseH = (RENDER_H / transY);
            int spriteH = std::max( 1, int( std::fabs( baseH * col.scale ) ) );
            int spriteW = (texture.height > 0) ? std::max( 1, int( spriteH * (float( texture.width ) / float( texture.height )) ) ) : spriteH;
            int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f );

            int y0 = bottomY - spriteH;
            int y1 = bottomY - 1;
            int x0 = -spriteW / 2 + spriteScreenX;
            int x1 = spriteW / 2 + spriteScreenX - 1;

            int cy0 = std::max( 0, y0 );
            int cy1 = std::min( RENDER_H - 1, y1 );
            int cx0 = std::max( 0, x0 );
            int cx1 = std::min( RENDER_W - 1, x1 );
            if (cy0 > cy1 || cx0 > cx1) continue;

            float invSpriteH = 1.0f / std::max( 1, spriteH );
            float invSpriteW = 1.0f / std::max( 1, spriteW );
            for (int sx = cx0; sx <= cx1; ++sx)
            {
                if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;
                float u = float( sx - x0 ) * invSpriteW;
                int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );
                for (int sy = cy0; sy <= cy1; ++sy)
                {
                    float v = float( sy - y0 ) * invSpriteH;
                    int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );
                    Uint32 color = texture.sample( texX, texY );
                    if (!boolIsNearBlack( color, 120 ))
                    {
                        float shade = 1.0f;
                        if (engineContext.caveMode)
                        {
                            shade = std::clamp( 1.0f / (0.35f * transY), 0.20f, 1.0f );
                            shade *= caveLight( transY );
                        }
                        else
                        {
                            shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * transY + engineContext.indoorShadeQuadratic * transY * transY);
                            shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                            shade *= 0.78f;
                        }
                        putPix( engineContext, sx, sy, applyAmbience( color, shade ) );
                    }
                }
            }
        }
    }

  

    /*
    for (auto &col : engineContext.columns)
    {
        float dx = col.x - engineContext.positionX;
        float dy = col.y - engineContext.positionY;
        col.distance = dx * dx + dy * dy; // Use squared distance
    }

    std::sort( engineContext.columns.begin(), engineContext.columns.end(), []( const ColumnProp &a, const ColumnProp &b ) {
        return a.distance > b.distance; // Farthest first
        } );

    for (const auto &col : engineContext.columns)
    {
        // Find the sprite set for this column
        auto setIt = engineContext.columnSpriteSets.find( col.setName );
        if (setIt == engineContext.columnSpriteSets.end() || setIt->second.numViews == 0)
        {
            continue; // This column has no valid sprite set, skip rendering
        }
        const SpriteSet &spriteSet = setIt->second;
        const int numViews = spriteSet.numViews;

        // Vector from player to column
        float vecX = col.x - engineContext.positionX;
        float vecY = col.y - engineContext.positionY;

        float vecX_to_Player = engineContext.positionX - col.x;
        float vecY_to_Player = engineContext.positionY - col.y;

        // Angle from column's center to the player
        float relativeAngle = std::atan2( vecY_to_Player, vecX_to_Player );

        // Normalize angle to [0, 2*PI]
        while (relativeAngle < 0) relativeAngle += 2.0f * 3.14159265f;
        while (relativeAngle >= 2.0f * 3.14159265f) relativeAngle -= 2.0f * 3.14159265f;

        // Map normalized angle to sprite index
        // We add slice*0.5 to offset the start, so 0 degrees is centered on the "front" sprite
        float slice = (2.0f * 3.14159265f) / numViews;
        int viewIndex = static_cast<int>( (relativeAngle + slice * 0.5f) / slice ) % numViews;

        const auto &texture = spriteSet.views[ viewIndex ];


        // Camera space
        float dx = col.x - engineContext.positionX;
        float dy = col.y - engineContext.positionY;
        float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
        float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
        float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
        if (transY <= 0) continue; // Behind player

        int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
        float baseH = (RENDER_H / transY);
        int spriteH = std::max( 1, int( std::fabs( baseH * col.scale ) ) );

        // Calculate width based on texture's aspect ratio
        int spriteW = spriteH;
        if (texture.height > 0)
        {
            spriteW = std::max( 1, int( spriteH * (float( texture.width ) / float( texture.height )) ) );
        }

        int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f ); // Aligns bottom with floor

        int y0 = bottomY - spriteH;
        int y1 = bottomY - 1;
        int x0 = -spriteW / 2 + spriteScreenX;
        int x1 = spriteW / 2 + spriteScreenX - 1;

        int cy0 = std::max( 0, y0 );
        int cy1 = std::min( RENDER_H - 1, y1 );
        int cx0 = std::max( 0, x0 );
        int cx1 = std::min( RENDER_W - 1, x1 );
        if (cy0 > cy1 || cx0 > cx1) continue;

        float invSpriteH = 1.0f / std::max( 1, spriteH );
        float invSpriteW = 1.0f / std::max( 1, spriteW );

        for (int sx = cx0; sx <= cx1; ++sx)
        {
            if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;

            float u = float( sx - x0 ) * invSpriteW;
            int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );

            for (int sy = cy0; sy <= cy1; ++sy)
            {
                float v = float( sy - y0 ) * invSpriteH;
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!boolIsNearBlack( color, 120 )) // Use existing transparency check
                {
                    // Apply cave lighting / distance fog
                    float shade = caveLight( transY );
                    color = shadeCol( color, shade );
                    putPix( engineContext, sx, sy, color );
                }
            }
        }
    }
    */


}

static void renderGameplayUiPass( Engine &engineContext ) {
    const bool overlayBusy = g_interactionAnim.active || g_levelTransition.active || g_notesOpen || g_codeEntryActive || g_caveQuizActive || g_levelEditorMode || g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive || g_solventLabUnlockCutsceneActive || g_solventCoolerEntryActive || g_redPigmentDispenseCutsceneActive;
    const bool cutsceneHudSuppressed = g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive || g_solventLabUnlockCutsceneActive || g_redPigmentDispenseCutsceneActive;
    const float shotFx01 = std::clamp( g_revolverRecoilTimer / std::max( 0.001f, kRevolverRecoilDuration ), 0.0f, 1.0f );
    const float museumDarknessAdd = museumPowerDarknessAddForLevel( engineContext.currentLevel );

    int lookingAtArt = pickArtworkUnderCrosshair( engineContext );

    float distanceToArt = 0.0f;
    if (lookingAtArt != -1)
    {
        const Artwork *art = nullptr;
        for (const auto &artWork : engineContext.artworks)
        {
            if (artWork.id == lookingAtArt)
            {
                art = &artWork;
                break;
            }
        }
        if (art)
        {
            float dx = (art->wx + 0.5f) - engineContext.positionX;
            float dy = (art->wy + 0.5f) - engineContext.positionY;
            distanceToArt = std::sqrt( dx * dx + dy * dy );
        }
    }

    if (!overlayBusy && lookingAtArt != -1 && engineContext.placardOpen == false && engineContext.journalOpen == false && distanceToArt < 2.5f)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 50, (RENDER_H / 2) + 5, "[E] To View", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.inRangeOfStatue && !engineContext.statueChatActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 70, (RENDER_H / 2) + 25, "[E] To Talk", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.showHelp)
    {
        drawString16x16( engineContext, 10, RENDER_H - 40, "[N] Notes", rgb( 200, 200, 120 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::CAVE && !g_caveQuizPassed && !g_caveQuizActive)
    {
        drawStringTinyScaled( engineContext, 12, RENDER_H - 20, "Logs Hold Clues", rgb( 170, 180, 210 ), 1, 1, 1, false );
    }

    renderRevolverShotEffects( engineContext, shotFx01 );
    renderSchoolSafeWeaponBlur( engineContext );

    const float defaultDarkness = std::clamp( g_horrorDarknessOverlay + (engineContext.caveMode ? 0.05f : 0.0f) + museumDarknessAdd, 0.0f, 0.58f );
    const float darknessAlpha = (g_wakeCutsceneActive && g_wakeDarknessOverride >= 0.0f)
        ? std::clamp( g_wakeDarknessOverride, 0.0f, 1.0f )
        : defaultDarkness;
    drawTranslucentBox(
        engineContext,
        0,
        0,
        RENDER_W,
        RENDER_H,
        rgb( 0, 0, 0 ),
        darknessAlpha );

    if (g_solventLabUnlockWhiteFlash > 0.0f)
    {
        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 255, 255 ), std::clamp( g_solventLabUnlockWhiteFlash, 0.0f, 1.0f ) );
    }

    if (g_wakeCutsceneActive)
    {
        const float p = std::clamp( g_wakeCutsceneTimer / std::max( 0.001f, kWakeCutsceneDuration ), 0.0f, 1.0f );
        const float eyeOpen = std::clamp( std::pow( p, 1.9f ), 0.0f, 1.0f );
        const int lidH = int( (RENDER_H * 0.5f) * (1.0f - eyeOpen) );

        if (lidH > 0)
        {
            drawTextBox( engineContext, 0, 0, RENDER_W, lidH, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
            drawTextBox( engineContext, 0, RENDER_H - lidH, RENDER_W, lidH, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
        }

        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.55f - p * 0.55f, 0.0f, 0.55f ) );

        if (p > 0.38f && p < 0.85f)
        {
            drawStringTinyScaled( engineContext, (RENDER_W / 2) - 45, RENDER_H - 44, "...where am I?", rgb( 185, 185, 200 ), 2, 1, 1, false );
        }

        if (p > 0.85f)
        {
            drawStringTinyScaled(engineContext, (RENDER_W / 2) - 150, RENDER_H - 44, "Wait, what was that thing? I should probably check the generator.", rgb(185, 185, 200), 2, 1, 1, false);
        }
    }

    drawStringTinyScaled( engineContext, 12, RENDER_H - 20, "X: " + to_string( engineContext.positionX ) + " " + "Y: " + to_string( engineContext.positionY ), rgb( 0, 0, 0 ), 1, 1, 1, false );
    {
        int fpsInt = (int)(engineContext.fps + 0.5f);
        drawStringTinyScaled( engineContext, 12, RENDER_H - 50, string( "FPS: " ) + to_string( fpsInt ), rgb( 200, 200, 200 ), 1, 1, 1, false );
    }

    int nearbyKey = getNearbyKeyPickup( engineContext );
    if (!overlayBusy && nearbyKey >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 45, "[E] Interact", rgb( 255, 240, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbyNote = getNearbyClueNote( engineContext );
    if (!overlayBusy && nearbyNote >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 85, "[E] Collect", rgb( 220, 225, 180 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGasCan( engineContext ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 122, (RENDER_H / 2) + 125, "[E] Pick Up Gas Can", rgb( 235, 215, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGenerator( engineContext ))
    {
        std::string prompt = "[E] Check Generator";
        if (g_generatorFueled)
        {
            prompt = "Generator Running";
        }
        else if (g_gasCanCollected)
        {
            prompt = "[E] Fill Generator";
        }
        drawString16x16( engineContext, (RENDER_W / 2) - 122, (RENDER_H / 2) + 145, prompt, rgb( 255, 240, 170 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySafe = getNearbySafe( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySafe >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] Open Safe", rgb( 180, 210, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySymbol = getNearbySymbol( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySymbol >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] Examine Pedestal", rgb( 250, 180, 250 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && isPlayerNearDirectorDesk( engineContext ) && !g_directorDeskUnlocked)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 150, (RENDER_H / 2) + 125, "[F] Unlock Director's Desk", rgb( 230, 200, 150 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::CAVE && isPlayerNearCaveStatue( engineContext ) && !g_caveQuizActive)
    {
        std::string statuePrompt = g_caveQuizPassed ? "WARDEN: PATH OPEN" : "[E] ANSWER WARDEN QUESTIONS";
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, statuePrompt, rgb( 210, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearPoint( engineContext, kUpperEntryX, kUpperEntryY, kUpperEntryRadius ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 140, (RENDER_H / 2) + 105, "[E] Go To Upper Gallery", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM_UPPER && isPlayerNearPoint( engineContext, 3.5f, 9.3f, 1.1f ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, "[E] Back To Ground Floor", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }
    if (!overlayBusy && isPlayerNearSolventCooler( engineContext ) && !g_playerKeys.contains( "RED PIGMENT" ) && !g_redPigmentDispenseCutsceneActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 160, (RENDER_H / 2) + 145, "[E] Use Solvent Cooler", rgb( 180, 235, 235 ), RENDER_W, 1, 2, true, rgb( 12, 25, 30 ) );
    }
    int doorTx = 0, doorTy = 0;
    if (isMuseumLikeLevel( engineContext.currentLevel ) && getDoorAheadTile( engineContext, doorTx, doorTy ))
    {
        int lockIndex = findDoorLockIndex( engineContext.currentLevel, doorTx, doorTy );
        if (!overlayBusy && lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked && !g_codeEntryActive)
        {
            const auto &lock = g_roomLocks[ lockIndex ];
            std::string req = (lock.type == LockType::KEY) ? ("[F] Use " + lock.requirement) : "[F] Enter 4-Digit Code";
            drawString16x16( engineContext, (RENDER_W / 2) - 110, (RENDER_H / 2) + 65, req, rgb( 255, 210, 100 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
        }
    }
    if (engineContext.currentLevel == Levels::MUSEUM_UPPER && getDoorAheadTile( engineContext, doorTx, doorTy ) && isRestorationGateDoorTile( doorTx, doorTy ) && !g_restorationWingUnlocked)
    {
        std::string req = hasRestorationPigments()
            ? "[F] Unseal Restoration Wing"
            : "[F] Missing Key Items";
        drawString16x16( engineContext, (RENDER_W / 2) - 155, (RENDER_H / 2) + 65, req, rgb( 255, 170, 170 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    const Artwork *art = nullptr;
    if (engineContext.openArtId >= 0)
    {
        for (const auto &artWork : engineContext.artworks)
        {
            if (artWork.id == engineContext.openArtId)
            {
                art = &artWork;
                break;
            }
        }
    }

    if (!cutsceneHudSuppressed)
    {
        renderCompass( engineContext );
    }

    if (art)
    {
        renderPolishedPlacard( engineContext );
    }
    if (engineContext.statueChatActive)
    {
        renderStatueChatbox( engineContext );
    }

    if (!cutsceneHudSuppressed && (engineContext.currentLevel == Levels::MUSEUM || engineContext.currentLevel == Levels::MUSEUM_UPPER))
    {
        renderObjectives( engineContext );
        renderGalleryCard( engineContext );
    }

    renderCaveHUD( engineContext );
    if (g_endGameStateAllowRevolver) renderHeldRevolver( engineContext );

    renderLevelEditorOverlay( engineContext );

    if (!overlayBusy) renderAccessPopup( engineContext );
    if (!cutsceneHudSuppressed)
    {
        if (g_endGameStateAllowRevolver) renderCombatHUD( engineContext );
    }
    renderNotesScreen( engineContext );
    renderCodeEntry( engineContext );
    renderSafeEntry( engineContext );
    renderSymbolEntry( engineContext );
    renderSolventCoolerEntry( engineContext );
    renderCaveQuiz( engineContext );
    renderInteractionAnimation( engineContext );
    renderLevelTransitionOverlay( engineContext );
    renderDialogueSubtitle( engineContext );
}
static void renderMenu(
    Engine &engineContext,
    int selection,
    float volume,
    bool musicOn,
    bool viewBob,
    int antiAliasingMode,
    int modelQualityPreset,
    int gpuRenderMode,
    bool multithreadingEnabled,
    unsigned int detectedThreadCount,
    bool schoolMode ) {
    // Dimensions
    int width = 460, height = 376;
    int x = (RENDER_W - width) / 2;
    int y = (RENDER_H - height) / 2;

    // Colors
    Uint32 bgCol = rgb( 25, 25, 30 );
    Uint32 borderCol = rgb( 180, 150, 50 );
    Uint32 textCol = rgb( 160, 160, 170 );
    Uint32 selCol = rgb( 255, 230, 100 );

    drawTextBox( engineContext, x, y, width, height, bgCol, borderCol );
    drawTextBox( engineContext, x + 4, y + 4, width - 8, height - 8, bgCol, borderCol );

    // Title Scaling
    std::string title = "Still Life";
    int scale = 3;
    // Tiny font is 3px wide * scale + 2px spacing
    int titleW = (int)title.length() * (3 * scale + 2);
    int titleX = x + (width - titleW) / 2;
    int titleY = y + 25;

    drawStringTinyScaled( engineContext, titleX, titleY, title, borderCol, scale, 2, 2, true );

    // Subtitle
    std::string sub = "Finally, a medium that breathes";
    int subX = (x + (width - (int)sub.length() * 6) / 2) - 40;
    drawStringTinyScaled( engineContext, subX, titleY + 25, sub, textCol, 2, 3, 1, false );


    int optY = y + 86;
    int lineH = 24;

    bool showCursor = (SDL_GetTicks() / 350) % 2 == 0;

    auto drawItem = [&]( int index, std::string label ) {
        bool isSel = (selection == index);
        Uint32 col = isSel ? selCol : textCol;

        std::string prefix = (isSel && showCursor) ? "> " : "  ";
        std::string suffix = (isSel && showCursor) ? " <" : "  ";
        std::string fullText = prefix + label + suffix;

    
        int charAdv = 12;

        int textW = (int)fullText.length() * charAdv;
        int textX = x + (width - textW) / 2;

        drawString16x16( engineContext, textX, optY + (index * lineH), fullText, col, width, 1, 2, true, rgb( 10, 10, 10 ) );
        };

    drawItem( 0, "Play" );

    std::string musicState = musicOn ? "ON" : "OFF";
    drawItem( 1, "Enable Audio: " + musicState );

    std::string volStr;
	static bool resetToDetect = false;
    if ((int)volume > 0 && volume != config::calibratedVolume) {
        volStr = std::to_string((int)volume) + "%";
    }
    else {
		volStr = "DETECT";
    }


    drawItem( 2, "Music Volume: " + volStr );

    std::string viewBobEnabler = viewBob ? "ON" : "OFF";
    drawItem(3, "Cinematic Camera: " + viewBobEnabler);

    std::string antiAliasingLabel = "LINEAR";
    if (antiAliasingMode == 0) antiAliasingLabel = "OFF";
    else if (antiAliasingMode == 2) antiAliasingLabel = "FXAA";
    drawItem( 4, "Anti-Aliasing: " + antiAliasingLabel );

    std::string quality = "BALANCED";
    if (modelQualityPreset == 0) quality = "HIGH";
    else if (modelQualityPreset == 2) quality = "PERFORMANCE";
    drawItem( 5, "Model Quality: " + quality );

    std::string gpuMode = "SMART";
    if (gpuRenderMode == 0) gpuMode = "NONE";
    else if (gpuRenderMode == 2) gpuMode = "FULL";
    drawItem( 6, "GPU Rendering: " + gpuMode );

    drawItem( 7, std::string( "Multithreading: " ) + (multithreadingEnabled ? "ON" : "OFF") + " (" + std::to_string( std::max( 1u, detectedThreadCount ) ) + " Threads)" );

    drawItem( 8, std::string( "School Mode: " ) + (schoolMode ? "ON" : "OFF") );

    drawItem( 9, "Quit" );

    std::string footer = "UP/DOWN Select    ENTER Confirm";
    int footW = (int)footer.length() * 4;
    drawStringTinyScaled( engineContext, x + (width - footW) / 2, y + height - 20, footer, rgb( 80, 80, 90 ), 1, 1, 1, false );
}

static void applyPresentationFilter( Engine &engineContext ) {
    if (!engineContext.backtexure) return;
    SDL_ScaleMode mode = (getAntiAliasingMode() == 0) ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
    (void)SDL_SetTextureScaleMode( engineContext.backtexure, mode );
}

static void applyPostAAMode( Engine &engineContext ) {
    if (getAntiAliasingMode() != 2) return;
    if (engineContext.backbuffer.size() != size_t( RENDER_W * RENDER_H )) return;

    static std::vector<Uint32> scratch;
    scratch.resize( engineContext.backbuffer.size() );
    scratch = engineContext.backbuffer;

    auto lum = []( Uint32 c ) -> float {
        const float r = float( (c >> 16) & 255 );
        const float g = float( (c >> 8) & 255 );
        const float b = float( c & 255 );
        return 0.299f * r + 0.587f * g + 0.114f * b;
    };

    for (int y = 1; y < RENDER_H - 1; ++y)
    {
        for (int x = 1; x < RENDER_W - 1; ++x)
        {
            const int i = y * RENDER_W + x;
            const Uint32 c = scratch[ i ];

            const float gx = std::fabs( lum( scratch[ i + 1 ] ) - lum( scratch[ i - 1 ] ) );
            const float gy = std::fabs( lum( scratch[ i + RENDER_W ] ) - lum( scratch[ i - RENDER_W ] ) );
            const float edge = gx + gy;
            if (edge < 26.0f) continue;

            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;
            for (int oy = -1; oy <= 1; ++oy)
            {
                for (int ox = -1; ox <= 1; ++ox)
                {
                    const Uint32 s = scratch[ (y + oy) * RENDER_W + (x + ox) ];
                    sumR += float( (s >> 16) & 255 );
                    sumG += float( (s >> 8) & 255 );
                    sumB += float( s & 255 );
                }
            }

            const float avgR = sumR / 9.0f;
            const float avgG = sumG / 9.0f;
            const float avgB = sumB / 9.0f;

            const float blend = std::clamp( (edge - 26.0f) / 84.0f, 0.0f, 0.45f );
            const float srcR = float( (c >> 16) & 255 );
            const float srcG = float( (c >> 8) & 255 );
            const float srcB = float( c & 255 );

            const Uint8 outR = Uint8( std::clamp( srcR * (1.0f - blend) + avgR * blend, 0.0f, 255.0f ) );
            const Uint8 outG = Uint8( std::clamp( srcG * (1.0f - blend) + avgG * blend, 0.0f, 255.0f ) );
            const Uint8 outB = Uint8( std::clamp( srcB * (1.0f - blend) + avgB * blend, 0.0f, 255.0f ) );
            engineContext.backbuffer[ i ] = rgb( outR, outG, outB );
        }
    }
}

static void renderModernCrosshairOverlay( Engine &engineContext ) {
    if (!engineContext.renderer) return;

    const int screenW = RENDER_W * WIN_SCALE;
    const int screenH = RENDER_H * WIN_SCALE;
    const int cx = screenW / 2;
    const int cy = screenH / 2;
    const int dotSize = 4;
    const int dotHalf = dotSize / 2;

    SDL_SetRenderDrawBlendMode( engineContext.renderer, SDL_BLENDMODE_BLEND );

    SDL_SetRenderDrawColor( engineContext.renderer, 0, 0, 0, 150 );
    SDL_FRect borderRect{
        float( cx - dotHalf - 1 ),
        float( cy - dotHalf - 1 ),
        float( dotSize + 2 ),
        float( dotSize + 2 )
    };
    SDL_RenderFillRect( engineContext.renderer, &borderRect );

    SDL_SetRenderDrawColor( engineContext.renderer, 255, 255, 255, 200 );
    SDL_FRect dotRect{
        float( cx - dotHalf ),
        float( cy - dotHalf ),
        float( dotSize ),
        float( dotSize )
    };
    SDL_RenderFillRect( engineContext.renderer, &dotRect );
}

static void renderModernRevolverHudOverlay( Engine &engineContext ) {
    if (!engineContext.renderer || !g_combatState.active || !g_combatState.hasRevolver || !g_endGameStateAllowRevolver) return;

    SDL_SetRenderDrawBlendMode( engineContext.renderer, SDL_BLENDMODE_BLEND );

    const float centerX = float( RENDER_W * WIN_SCALE - 60 );
    const float centerY = float( RENDER_H * WIN_SCALE - 60 );
    const float ringRadius = 18.0f;
    const float chamberSize = 7.0f;
    const int loaded = std::clamp( g_combatState.loadedAmmo, 0, 6 );
    constexpr float kTau = 6.28318530718f;

    for (int i = 0; i < 6; ++i)
    {
        const float angle = (-kTau * 0.25f) + (kTau * (float)i / 6.0f);
        const float px = centerX + std::cos( angle ) * ringRadius;
        const float py = centerY + std::sin( angle ) * ringRadius;

        SDL_FRect chamberRect{
            px - (chamberSize * 0.5f),
            py - (chamberSize * 0.5f),
            chamberSize,
            chamberSize
        };

        if (i < loaded)
        {
            SDL_SetRenderDrawColor( engineContext.renderer, 255, 220, 100, 255 );
            SDL_RenderFillRect( engineContext.renderer, &chamberRect );
        }
        else
        {
            SDL_SetRenderDrawColor( engineContext.renderer, 50, 50, 50, 100 );
            SDL_RenderFillRect( engineContext.renderer, &chamberRect );
        }

        SDL_SetRenderDrawColor( engineContext.renderer, 25, 25, 25, i < loaded ? 190 : 135 );
        SDL_RenderRect( engineContext.renderer, &chamberRect );
    }
}



