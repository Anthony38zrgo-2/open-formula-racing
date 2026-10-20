#include "drivers.h"
#include <cstddef>

// References:
// https://en.wikipedia.org/wiki/List_of_most_popular_given_names
// https://en.wikipedia.org/wiki/Lists_of_most_common_surnames_in_Asian_countries

static const char *JP_surnames[] =
{
    "Sato",
    "Suzuki",
    "Takahashi",
    "Tanaka",
    "Watanabe",
    "Ito",
    "Nakamura",
    "Kobayashi",
    "Yamamoto",
    "Kato",
    "Yoshida",
    "Yamada",
    "Sasaki",
    "Yamaguchi",
    "Matsumoto",
    "Inoue",
    "Kimura",
    "Shimizu",
    "Hayashi",
    "Saito",
    "Yamazaki",
    "Nakajima",
    "Mori",
    "Abe",
};

static const char *JP_names[] =
{
    "Aoi",
    "So",
    "Ao",
    "Nagi",
    "Nagisa",
    "Ren",
    "Haruto",
    "Hinato",
    "Minato",
    "Soma",
    "Fuma",
    "Ao",
    "Aoi",
    "Itsuki",
    "Tatsuki",
    "Yamato",
    "Yuma",
    "Haruma",
    "Dan",
    "Haru",
};

static const char *DE_surnames[] =
{
    "Mueller",
    "Schmidt",
    "Schneider",
    "Fischer",
    "Weber",
    "Meyer",
    "Wagner",
    "Becker",
    "Schulz",
    "Hoffmann",
    "Schafer",
    "Koch",
    "Bauer",
    "Richter",
    "Klein",
    "Wolf",
    "Schroeder",
    "Neumann",
    "Schwarz",
    "Zimmermann",
    "Braun",
    "Krueger",
    "Hofmann",
    "Hartmann",
    "Lange",
};

static const char *DE_names[] =
{
    "Noah",
    "Matteo",
    "Elias",
    "Finn",
    "Leon",
    "Theo",
    "Paul",
    "Emil",
    "Henry",
    "Ben",
};

static const char *GB_names[] =
{
    "Noah",
    "Muhammad",
    "George",
    "Oliver",
    "Leo",
    "Arthur",
    "Oscar",
    "Theodore",
    "Freddie",
    "Theo",
};

static const char *GB_surnames[] =
{
    "Smith",
    "Johnson",
    "Williams",
    "Brown",
    "Jones",
    "Miller",
    "Davis",
    "Garcia",
    "Rodriguez",
    "Wilson",
    "Martinez",
    "Anderson",
    "Taylor",
    "Thomas",
    "Hernandez",
    "Moore",
    "Martin",
    "Jackson",
    "Thompson",
    "White",
    "Lopez",
    "Lee",
    "Gonzalez",
    "Harris",
    "Clark",
};

static const char *const CR_names[] =
{
    "Mia",
    "Nika",
    "Rita",
    "Mila",
    "Ema",
    "Lucija",
    "Marta",
    "Sara",
    "Eva",
    "Elena",
    "Luka",
    "Jakov",
    "David",
    "Mateo",
    "Toma",
    "Roko",
    "Petar",
    "Matej",
    "Fran",
    "Ivan",
};

static const char *const CR_surnames[] =
{
    "Horvat",
    "Kovacevic",
    "Babic",
    "Maric",
    "Juric",
    "Novak",
    "Kovacic",
    "Knezevic",
    "Vukovic",
    "Markovic",
    "Petrovic",
    "Matic",
    "Tomic",
    "Pavlovic",
    "Kovac",
    "Bozic",
    "Blazevic",
    "Grgic",
    "Pavic",
    "Radic",
    "Peric",
    "Filipovic",
    "Saric",
    "Lovric",
    "Vidovic",
    "Perkovic",
    "Popovic",
    "Bosnjak",
    "Jukic",
    "Barisic",
};

static const char *const IT_names[] =
{
    "Sofia",
    "Aurora",
    "Giulia",
    "Ginevra",
    "Vittoria",
    "Beatrice",
    "Alice",
    "Ludovica",
    "Emma",
    "Matilde",
    "Leonardo",
    "Francesco",
    "Tommaso",
    "Edoardo",
    "Alessandro",
    "Lorenzo",
    "Mattia",
    "Gabriele",
    "Riccardo",
    "Andrea",
};

static const char *const IT_surnames[] =
{
    "Rossi",
    "Russo",
    "Ferrari",
    "Esposito",
    "Bianchi",
    "Romano",
    "Colombo",
    "Bruno",
    "Ricci",
    "Greco",
    "Marino",
    "Gallo",
    "De Luca",
    "Conti",
    "Costa",
    "Mancini",
    "Giordano",
    "Rizzo",
    "Lombardi",
    "Barbieri",
    "Moretti",
    "Fontana",
    "Caruso",
    "Mariani",
    "Ferrara",
};

static const char *const FR_names[] =
{
    "Louise",
    "Ambre",
    "Alba",
    "Jade",
    "Emma",
    "Rose",
    "Alma",
    "Alice",
    "Romy",
    "Anna",
    "Gabriel",
    "Raphael",
    "Leo",
    "Louis",
    "Mael",
    "Noah",
    "Jules",
    "Adam",
    "Arthur",
    "Isaac",
};

static const char *const FR_surnames[] =
{
    "Martin",
    "Bernard",
    "Dubois",
    "Thomas",
    "Robert",
    "Richard",
    "Petit",
    "Durand",
    "Leroy",
    "Moreau",
    "Simon",
    "Laurent",
    "Lefebvre",
    "Michel",
    "Garcia",
    "David",
    "Bertrand",
    "Roux",
    "Vincent",
    "Fournier",
    "Morel",
    "Girard",
    "Andre",
    "Lefevre",
    "Mercier",
};

#define NATION(X) \
{ \
    #X, \
    X##_names, \
    X##_surnames, \
    sizeof X##_names / sizeof *X##_names, \
    sizeof X##_surnames / sizeof *X##_surnames \
}

const struct GfDrivers::names GfDrivers::names[] =
{
    NATION(JP),
    NATION(GB),
    NATION(DE),
    NATION(CR),
    NATION(IT),
    NATION(FR),
};

const char *const GfDrivers::teams[] =
{
    "Rocketeer Racing",
    "Oli's Oil Team",
    "Velocity Racing",
    "Lovely BEAM",
    "Chaser Racing",
    "Nitro Racing",
    "Haruki Racing",
    "Iadanza Racing",
    "Asai Racing",
    "Japanese Toro Drivers Club",
    "Anime Age Racing",
    "Free French Racers team Orcana",
    "Speedstar Racing",
    "Takasuka Racing",
};

const size_t GfDrivers::n_names = sizeof names / sizeof *names;
const size_t GfDrivers::n_teams = sizeof teams / sizeof *teams;
