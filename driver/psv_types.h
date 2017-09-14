/**
 * Motivation: One unified .psv format for archiving (preserving) Vita games.
 * The goal is to preserve as much of the original game structure while ensuring
 * the all the information needed to decrypt and extract data can be derived
 * from just the file and a hacked Vita.
 *
 * We want something akin to .nds or .3ds/.cia or .iso but for Vita games. The
 * unique challenge is that Vita cart games require a per-cart key to decrypt
 * and digital games require a similar key from activation. With just the raw
 * game image, it is not possible to extract the game data.
 *
 * What's wrong with using .vpk? VPK is designed for homebrew. The patches to
 * enable homebrew strips out a lot of the game executable metadata as well as
 * change the system state to be different than a Vita running an original game.
 * This leads to many subtle as well as major bugs (saves not working, some
 * games require additional patches to run, saves are not compatible with
 * non-hacked Vitas, etc).
 *
 * Why not just ZIP the original files? Why not strip PFS as well to make data
 * mining/emulation easy? Why not make a compressed format? One reason is that
 * by stripping more than necessary (like, for example PFS), we might be losing
 * information that we currently do not think is important. An example of this
 * is when SNES games are first dumped and Earthbound was not dumped properly
 * and people did not know about the anti-piracy checks until much later. There
 * may be, for example, games that do timing checks or checks on the file
 * modification time or something. Either explicitly for anti-piracy or
 * implicitly due to bad programming (a lot of older consoles are infamous for
 * the latter case). By preserving as much of the original structure as
 * possible, we ensure that we can somehow play these games in a future where no
 * more Vitas exist.
 *
 * Different tools (data extraction, backup loaders, archival storage, etc)
 * might require different use cases. Someone might for example want to strip
 * PFS and compress the game data for more efficient storage. We invite them to
 * extend this format though flags BUT just as you shouldn't store all your
 * photos in level-9 compressed JPEG, your code in executables, or any data you
 * care about in a lossy format, you should archive your games in its original
 * form. You can easily go from a RAW image to a JPEG but you cannot go back.
 */

// active discussion on the format goes here
// https://gist.github.com/yifanlu/d546e687f751f951b1109ffc8dd8d903

//format authors:
//yifanlu
//motoharu
//devnoname120

#pragma once

#pragma pack(push, 1)

typedef struct digital_header_t
{
  uint32_t type; // 0x1 indicates header for digital content
  uint32_t flags; // 1 == game, 2 == DLC, etc (not yet specified)
  uint64_t license_size; // size of RIF that follows
  uint8_t rif[]; // rif file
} digital_header_t;

typedef struct compression_header_t
{
  uint32_t type; // 0x2 indicates header for compression
  uint32_t compression_algorithm; // not yet specified
  uint64_t uncompressed_size;
} compression_header_t;

typedef union opt_header_t
{
  uint32_t type;
  digital_header_t digital;
  compression_header_t compression;
} opt_header_t;

typedef struct psv_file_header_base
{
  uint32_t magic;
  uint32_t version;
} psv_file_header_base;

typedef struct psv_file_header_v1
{
  uint32_t magic;               // 'PSV\0'
  uint32_t version;             // 0x00 = first version
  uint32_t flags;               // see below
  uint8_t key1[0x10];           // for klicensee decryption
  uint8_t key2[0x10];           // for klicensee decryption
  uint8_t signature[0x14];      // same as in RIF
  uint8_t hash[0x20];           // optional consistancy check. sha256 over complete data (including any trimmed bytes) if cart dump, sha256 over the pkg if digital dump.
  uint64_t image_size;          // if trimmed, this will be actual size
  uint64_t image_offset_sector; // image (dump/pkg) offset in multiple of 512 bytes. must be > 0 if an actual image exists. == 0 if no image is included.
  opt_header_t headers[];       // optional additional headers as defined by the flags
} psv_file_header_v1;

#define PSV_MAGIC (0x00565350) // 'PSV\0'

#define PSV_VERSION_V1 1

#define FLAG_TRIMMED (1 << 0)  // if set, the file is trimmed and 'image_size' is the actual size
#define FLAG_DIGITAL (1 << 1)  // if set, RIF is present and an encrypted PKG file follows
#define FLAG_COMPRESSED (1 << 2)  // undefined if set with `FLAG_TRIMMED` or `FLAG_DIGITAL`. if set, the data must start with a compression header (not currently defined)
#define FLAG_LICENSE_ONLY (FLAG_TRIMMED | FLAG_DIGITAL) // if set, the actual PKG is NOT stored and only RIF is present. 'image_size' will be size of actual package.

#pragma pack(pop)

/** 
 * Sample Usage 1: Game Cart Archival
 *   flag = 0, rif_size = 0, image_size = size of game dump, header is 
 *   followed by raw dump of game cart
 * Sample Usage 2: Save space of dump
 *   flag = FLAG_TRIMMED, rif_size = 0, image_size = size of game dump, 
 *   header is followed by trimmed dump (trailing zeros are not included)
 * Sample Usage 3: Digital content archival
 *   flag = FLAG_DIGITAL, rif_size = 0x200 (size of rif), image_size = 
 *   size of PKG from PSN servers, header is followed by RIF followed 
 *   by the game PKG
 * Sample Usage 4: Backup of license for digital content
 *   flag = FLAG_DIGITAL | FLAG_TRIMMED, rif_size = 0x200, image_size = 
 *   size of PKG from PSN servers, header is followed by RIF
 **/
