# In-game download manager specification

## Introduction

Speed Dreams features an in-game download manager to fetch assets, such as
cars, tracks or drivers, from external providers. Assets are listed from a
simple database on a known URL and fetched from a web server. The download
manager can be accessed from the main screen by pressing the `Downloads`
button.

This in-game download manager was loosely inspired by that from the
[FlightGear](https://flightgear.org/) project.

This has the following benefits:

- Assets can be developed independently from Speed Dreams i.e., on separate
repositories and possibly also by independent maintainers.
- As a free software project, Speed Dreams can only officially distribute
assets under a free license, such as the
[CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) or the
[Free Art License](https://artlibre.org/licence/lal/en/). However, users might
still want to install assets under non-free licenses, such as non-commercial
licenses.
- It allows Speed Dreams to contain only a minimum essential set of assets
on its source tree, thus reducing the repository size significantly.
    - Existing assets can be split into their own repositories, too.

Even if Speed Dreams configures the
[official assets database](https://www.speed-dreams.net/assets.json) by default,
this download manager has been designed to remain decentralised. This means any
web server compatible with the specification defined here can be added or
removed from the list.

## Asset database

In order for Speed Dreams to know which assets can be downloaded, a
[JSON](https://json.org/) database must be defined. A formal
[JSON schema](./assets.schema.json) and
[a practical example](./assets.example.json) are defined. A database can be
verified with the
[`check-jsonschema`](https://pypi.org/project/check-jsonschema/) utility:

```
check-jsonschema --schemafile assets.schema.json <file>
```

The database defines arrays for the following asset types:

- Cars
- Tracks
- Drivers

In JSON syntax:

```json
{
    "cars": [
    ],
    "tracks": [
    ],
    "drivers": [
    ],
}
```

Each one of the types listed above is optional. For example, `tracks` can be
ommitted if a database does not define any:

```json
{
    "cars": [
    ],
    "drivers": [
    ],
}
```

All entries inside `cars`, `drivers` or `tracks` follow the same schema:

```json
{
    "name": <string>,
    "category": <string>,
    "author": <string>,
    "license": <string>,
    "url": <string>,
    "hashtype": <string>,
    "hash": <string>,
    "size": <string>,
    "thumbnail": <string>,
    "directory": <string>,
    "revision": <string>
},
```

Where:

- `name`: defines the human-readable name for an asset.
- `category`: defines which category an asset belongs to.
    - For cars, the value is only shown for informational purposes, but no
    actions are made based on its value.
    - For tracks, the following values are accepted:
        - `circuit`
        - `development`
        - `dirt`
        - `gprix`
        - `karting`
        - `road`
        - `speedway`
    - For drivers, it should not match `human` or `networkhuman`.
- `author`: defines the author(s) for an asset. Speed Dreams shall not
perform any specific actions based on the value of this string.
- `license`: defines the license covering the asset. Speed Dreams shall not
perform any specific actions based on the value of this string.
- `url`: defines the URL that contains the asset. `http` and `https` are
currently the only supported protocols, but others might be added in the
future. The URL must point to a [ZIP file](https://en.wikipedia.org/wiki/.zip).
The download manager shall not require any file extension to the ZIP file.
The URL does not have to point to the same web server hosting the asset
database.
- `hashtype`: defines the hash function used to calculate the hash for the
asset. As of the time of this writing, only `sha256` is supported, but new
entries might be introduced in the future.
- `hash`: resulting hash as calculated by the hash function defined by
`hashtype`. The download manager shall reject downloaded assets whose
calculated hash does not match the expected hash defined here.
- `size`: asset size, in bytes. This value is expressed as a string so as to
restrict the use JSON numbers, as those are defined as floating-point values.
Subdivisions, such as KiB or MiB, are not allowed.
- `thumbnail`: defines the URL for the thumbnail that will be displayed on
the download manager. The URL must point to an image file with an extension
known to the `GfTexReadImageFromFile` function. The URL does not have to point
to the same web server hosting the asset database.
- `directory`: defines the directory where the asset shall be stored.
`localdir` is the path to a directory calculated at run-time by Speed Dreams,
and matches the value returned by the `GfLocalDir` function.
    - Cars are stored into `<localdir>/cars/models/<directory>`.
    - Tracks are stored into `<localdir>/tracks/<category>/<directory>`.
    - Drivers are stored into `<localdir>/drivers/<category>/<index>`.
    Since Speed Dreams refers to drivers by their indexes, once extracted
    it will rename `directory` to the next integer `index` available in the
    directory.
- `revision`: an integer that defines the revision of an asset. This value is
used as a basic mechanism to inform users about updates. The download manager
shall write the currently downloaded revision into a file called `.revision`
inside the asset directory.

> **TODO:** future releases will also store cars and tracks into `localdir`
> instead of `datadir`.

## Asset file structure

An asset must be defined as a [ZIP file](https://en.wikipedia.org/wiki/.zip)
with the following tree structure:

```
<directory>/
├── <file 0>
├── <file 1>
├── ...
└── <file n>
```

Where `directory` is the directory name defined by the `directory` key from
the assets database. Any files on the same level as the top-level directory
shall be ignored.

When an asset is downloaded or updated, the original directory is recursively
removed and replaced with the contents of the ZIP file, if it exists.

## Design and implementation

### Source files

The following files were created inside the
`src/modules/userinterface/legacymenu/mainscreens/` directory to implement the
download manager:

- `asset.cpp`
- `asset.h`
- `assets.cpp`
- `assets.h`
- `assets.h`
- `downloadservers.cpp`
- `downloadservers.h`
- `downloadsmenu.cpp`
- `downloadsmenu.h`
- `entry.cpp`
- `entry.h`
- `hash.cpp`
- `hash.h`
- `repomenu.cpp`
- `repomenu.h`
- `sha256.cpp`
- `sha256.h`
- `sink.cpp`
- `sink.h`
- `thumbnail.cpp`
- `thumbnail.h`
- `unzip.cpp`
- `unzip.h`
- `writebuf.cpp`
- `writebuf.h`
- `writefile.cpp`
- `writefile.h`

### Main classes

The following classes define the GUI menus related to the download manager:

#### `DownloadsMenu`

`downloadsmenu.cpp` and `downloadsmenu.h` implement the `DownloadsMenu` class,
which holds the main logic for the download manager. This class relies on the
rest of the files listed above to perform the following tasks:

- Download files to memory.
- Download files to the filesystem.
- Display thumbnails and other GUI elements.
- Unzipping downloaded assets.
- Calculate and verify hashes for downloaded assets.
- Fetch files from a web server.
- Fetch one or more assets databases and parse them.
- Keep track of the state for each asset (e.g.: fetching thumbnail, downloaded,
updates available, etc.).
- Organize assets into pages.

`DownloadsMenu::recompute` is called by the event loop handler so that
downloads can be updated. The `GfEventLoop` was modified so that the event
loop handler would call the user-defined callback with an additional parameter:
the maximum number of milliseconds the user-defined callback can spend before
returning to the caller.

This allows to update ongoing downloads by calling `curl_multi_poll` without
blocking the main thread, thus removing the need for a separate thread.

#### `RepoMenu`

`repomenu.cpp` and `repomenu.h` implement the `RepoMenu` class, which lists
the repositories defined by users into a scroll list and allows them to
add or remove repositories. This menu can be only entered from the
`Configure` button inside the downloads menu.

When this menu is exited, the downloads menu shall fetch the assets databases
again, based on the new configuration. However, assets still not completely
downloaded shall not be removed from the downloads menu.

### Auxiliary classes

#### `Asset`

`asset.cpp` and `asset.h` implement the `Asset` class, which is the binary
representation of a parsed entry from the assets database.

#### `Assets`

`assets.cpp` and `assets.h` implement the `Assets` class, a thin abstraction
layer of a `std::vector`. The `Assets` class shall read a JSON file and parse
the different arrays inside it to parse each asset individually by calling
`Asset::parse`. Then, a `std::vector` of `Asset` is constructed, that callers
can retrieve via a read-only reference.

#### `Thumbnail`

`thumbnail.cpp` and `thumbnail.h` implement the `thumbnail` class, which
defines one of the visible entries as shown on the downloads menu.

#### `entry`

`entry.cpp` and `entry.h` implement `struct entry`, a relatively simple data
structure containing information about an asset, such as the location of its
data and thumbnail in the filesystem, as well as its state.

Whereas the number of `thumbnail` objects is fixed and determined by the
`THUMBNAILS` enumerator on `downloadsmenu.cpp`, every asset defines its own
`entry` object, and might or might not be shown at a given moment, as this
depend on the filter settings and the active page.

#### `sink`

`sink.cpp` and `sink.h` implement the `sink` class, which is meant to be
derived to define how fetched files should be downloaded. `downloadsmenu.cpp`
would make use of pointers to `sink` objects so as to remain agnostic to how
files are actually downloaded, as this depends on the operation itself:

- Assets databases are downloaded to memory so that `cJSON` can parse them
via the `writebuf` class.
- Thumbnails and assets are downloaded to files via the `writefile` class.

`sink` defines a maximum size for a given file, so that there is no risk for
a download operation to cause resource exhaustion.

#### `writebuf`

`writebuf.cpp` and `writebuf.h` implement the `writebuf` class, derived from
`sink`, which implements file downloads to memory.

#### `writefile`

`writefile.cpp` and `writefile.h` implement the `writefile` class, derived
from `sink`, which implements file downloads to the filesystem.

#### `unzip`

`unzip.cpp` and `unzip.h` implement the `unzip` class, which is a thin,
high-level abstraction layer on top of the `minizip` library. As its name
suggests, the `unzip` class is responsible for taking a path to a ZIP file
as input and a destination path where its contents shall be written.

#### `hash`

`hash.cpp` and `hash.h` implement the `hash` abstract class, which defines a
common interface for all hash functions to implement.

#### `sha256`

`sha256.cpp` and `sha256.h` implement the `sha256` class, derived from the
`hash` abstract class, which implements the interface to run the
[SHA256](https://en.wikipedia.org/wiki/SHA-2) hash function.

### Auxiliary source files

#### `downloadservers.cpp`

This file, together with `downloadservers.h`, defines functions to read and
write the databases configuration file located on `config/downloadservers.xml`.

## Future improvements

- Perform hash calculation asynchronously i.e., update every time a chunk
is received from the network.
- Perform unzip operations asynchronously so as to avoid blocking the main
thread.
