# Disk structures

VID
: Volume Identification Directory. Always sector 0, length 1 sector.

SAT
: Sector Allocation Table / map. Variable length

SDB
: Secondary directory block (list of catalogues). Length 1 sector.

SDE
: Secondary directory block entry (catalogue entry)

PDB
: Primary Directory Block, list of filenames. Length 4 sectors.

PDE
: Primary Directory Entry, file entry

FAB
: File allocation block / list of data blocks. Variable length.

FABSD
: File Allocation Block Segment Descriptor

DB
: Data Block / block of sequential records. Variable length.

FILE
: Linear file

HDR
: Header (first few bytes of each SDB, PDB, or FAB); contains linkage
information to next structure of same type. FABs have forward and backward
links; other structures have only forward links.

SLT
: Sector Lockout Table.

DTA
: Diagnostic Test Areas.

PSN
: Pysical Sector Number

# Structural Overview

```
(sector 0)
/-------\    /-------\
|  VID  |--->|  SAT  |              [DISK HEADER & GLOBAL DATA]
\-------/    \-------/
    |
    v
/-------\    /-------\
|  SDB  |--->|  SDB  | ...          [USERS & DIRECTORIES]
|-------|    \-------/
| SDE.. |
\-------/
    |
    v
/-------\    /-------\
|  PDB  |--->|  PDB  | ...          [DIRECTORY CONTENTS]
|-------|    \-------/
| PDE.. |-
\-------/ \  /------\
    |      ->|  DB  |               [LINEAR FILES]
    |        \------/
    V
/-------\    /-------\
|  FAB  |--->|  FAB  | ...          [NON-LINEAR DATASETS]
|-------|    \-------/
|FABSD..|-
\-------/ \  /------\
           ->|  DB  |
             \------/
```

# Volume ID Block (VID)

always Sector 0, length 1 sector

| Symbol |  Offset |  Length | Description
|--------|---------|---------|------------
| VIDVOL |   0 $00 |   4 $04 | Volume ASCII identifier
| VIDUSN |   4 $04 |   2 $02 | User number
| VIDSAT |   6 $06 |   4 $04 | Start of SAT
| VIDSAL |  10 $0A |   2 $02 | Length of SAT
| VIDSDS |  12 $0C |   4 $04 | Secondary directory start
| VIDPDL*|  16 $10 |   4 $04 | Primary directory PSN list start
| VIDOSS |  20 $14 |   4 $04 | Start of CASE Program Load (IPL) file
| VIDOSL |  24 $18 |   2 $02 | CPL length
| VIDOSE |  26 $1A |   4 $04 | CPL execution address
| VIDOSA |  30 $1E |   4 $04 | CPL load address
| VIDDTE |  34 $22 |   4 $04 | Generation date
| VIDVD  |  38 $26 |  20 $14 | Volume Descriptor
| VIDVNO |  58 $3A |   4 $04 | Initial version/revision
| VIDCHK |  62 $3E |   2 $02 | VID checksum
| VIDDTP |  64 $40 |  64 $40 | Diagnostic test pattern
| VIDDTA | 128 $80 |   4 $04 | Diagnostic test area directory
| VIDDAS | 132 $84 |   4 $04 | Start of dump area
| VIDDAL | 136 $88 |   2 $02 | Length of dump area
| VIDSLT | 138 $8A |   4 $04 | Start of SLT
| VIDSLL | 142 $8E |   2 $02 | Length of SLT
| VIDRS1 | 144 $90 | 104 $68 | Reserved
| VIDMAC | 248 $F8 |   8 $08 | VERSAdos disk (ASCII string 'EXORmacs')

# Sector Allocation Table (SAT)

Sector Allocation Table / Map. Variable length.

One bit per sector of the disk.

# Secondary Directory Block (SDB)

Secondary directory block (list of catalogues), length 1 sector,
can be chained to additional SDB sectors if needed.


| Symbol |  Offset | Length | Description
|--------|---------|--------|------------
| SDBFPT |   0 $00 |  4 $04 | PSN of next SDB (zero if none)
|RESERVED|   4 $04 | 12 $0B | Reserved
| SDE1   |  16 $10 | 16 $10 | First catalogue
| SDE2   |  32 $20 | 16 $10 | Second catalogue
| ...
| SDE15  | 240 $F0 | 16 $10 | Catalogue #15

Each SDE entry contains a name and pointer for one catalogue of files

| Symbol |  Offset | Length | Description
|--------|---------|--------|------------
| SDBUSN |   0 $00 |  2 $02 | User Number
| SDBCLG |   2 $02 |  8 $08 | Catalogue name
| SDVPDP |  10 $0A |  4 $04 | PSN of first PDB for catalogue
| SDBAC1 |  14 $0E |  1 $01 | Reserved
| SDBRS1 |  15 $0F |  1 $01 | Reserved

# Primary Directory Block (PDB)

One catalogue of files. Each 4 sectors large with room for up to 20 files,
Chained to additional PDB blocks if needed.

| Symbol |  Offset | Length | Description
|--------|---------|--------|------------
| DIRFPT |   0 $00 |  4 $04 | PSN of next PDB (zero if none)
| DIRUSN |   4 $04 |  2 $02 | User number
| DIRCLG |   6 $06 |  8 $08 | Catalogue name
|        |  14 $0E |  2 $02 | Reserved
| PDE1   |  16 $10 | 50 $32 | First directory entry
| PDE2   |  64 $40 | 50 $32 | Second directory entry
| ...
| PDE20

## Primary Directory Entry

contains information about one file name


| Symbol | Offset | Length | Description
|--------|--------|--------|------------
| DIRFIL |  0 $00 |  8 $08 | Filename
| DIREXT |  8 $08 |  2 $02 | Extension
| DIRRSI | 10 $0A |  2 $02 | Reserved
| DIRFS  | 12 $0C |  4 $04 | File starting PSN (contiguous) or first FAB pointer (noncontiguous)
| DIRFE  | 16 $10 |  4 $04 | Physical EOF Logical Sector Number (LSN) (contiguous) or last FAB pointer (noncontiguous)
| DIREOF | 20 $14 |  4 $04 | End of file LSN (zero if contiguous)
| DIREOR | 24 $18 |  4 $04 | End of file Logical Record Number (LRN) (zero if contiguous)
| DIRWCD | 28 $1C |  1 $01 | Write access code
| DIRRCD | 29 $1D |  1 $01 | Read access code
| DIRAT  | 30 $1E |  1 $01 | File attributes
| DIRLBZ*| 31 $1F |  1 $01 | Last data block size (zero if contiguous)
| DIRLRL | 32 $20 |  2 $02 | Record size (zero if variable record length or contiguous)
| DIRRSZ | 34 $22 |  1 $01 | Reserved
| DIRKEY | 35 $23 |  1 $01 | Key size (zero if null keys or non-Indexed Sequential Access Mode (non-ISAM)
| DIRFAB | 36 $24 |  1 $01 | FAB size (zero if contiguous)
| DIRDAT | 37 $25 |  1 $01 | Data block size (zero if contiguous)
| DIRDTEC| 38 $26 |  2 $02 | Data file created or updated
| DIRDTEA| 40 $28 |  2 $02 | Last date file assigned
| DIRRS3 | 42 $2A |  8 $06 | Reserved

* In current implementation, DIRBLZ=DARDIT

File attributes/type (DIRATT) (bits 4-7 user defined attribtues)

| Symbol | Value | Description
|--------|-------|-------------
| DATCON |     0 | Contigous
| DATSEQ |     1 | Sequential (variable or fixed record length)
| DATISK |     2 | Keyed ISAM - No duplicate keys
| DATISD |     3 | Keyed ISAM - Duplicate keys allowed; null keys allowed

# File Access Block (FAB)

The File Access Block (FAB) is used by non-contigous files. Contains
additional data about the structured file type and pointers to the
individual segments of the file.

Chained to additional FAB blocks if needed using a doubly-linked list
allowing the OS to search the data in both directions.

| Symbol | Offset | Length | Description
|--------|--------|--------|------------
| FABFLK |  0 $00 |  4 $04 | Pointer to the next FAB (zero if none)
| FABBLK |  4 $04 |  4 $04 | Pointer to the previous FAB (zero if none)
| FABUSE |  8 $08 |  1 $01 | Number of FAB Segment Descriptors in use. 0-16.
| FABPKY*|  9 $09 | 1 $01  | Length of previous FAB's last key
|        | 10 $10 | FABPKY | Last key in previous FAB (zero length if this is the first FAB)
| FABSEG | ...    | ...    | Start of first segment descriptor

## FAB Segment Descriptor

Describes one data segment of the structured file

| Symbol | Offset | Length | Description
|--------|--------|--------|------------
| FABPSN |  0 $00 |  4 $04 | PSN of data block start (zero if rest of FAB empty)
| FABREC |  4 $04 |  2 $02 | Number of records in data block
| FABSGS |  6 $06 |  1 $01 | Size of data block in number of sectors
| FABKEY |  7 $07 |  1 $01 | Key of last record in data block

* Current implementation always has FABKEY = DIRKEY

** The key may not be equal to the last key in the data block, but it is
always less than the first key of next data block unless duplicate keys are
allowed. If in first FAB, FABPKY for first FAB has byte for byte
correspondence with length of keys established for file and values of
bytes are zeroes.

# Data Block (DB)

Data blocks can contain three different record types, none of which allows
a record to be split between data blocks:

 * FIXED LENGTH records
 * VARIABLE LENGTH records
 * CONTIGUOUS file

## Fixed Length Records:
The data content of the fixed length record is specified by the user. There
are no embedded control characters generated by File Management Service
(FMS). Any unused locations in the data block will be zeroed out.

## Variable Length Records:
Variable length records have a two-byte field, preceding each record, which
contains the number of data bytes, followed by the data. The data portion
of the record can be binary or ASCII. With ASCII specification and a
formatted write request, FMS will compress spaces. A space compression
character is indicated by a data byte having the sign bit (bit 1) set,
while the remaining bi ts (6-0) contain a binary number representing the
number of spaces to be inserted in place of the compressed character. FMS
will automatically expand these compressed characters into spaces when such
files are read, using formatted ASCII I/O. A zero filler byte is stored at
the end of the data portion of a record if the record length is odd. Any
unused locations in the da ta block will be zeroed.

## Contiguous Records:
Records for contiguous files (load modules) are 256 bytes in length. The
data content, completely specified by the user, contains no embedded
control characters generated by FMS.

# Deleted files

Deleted files have their first letter zeroed, and blocks marked as free in
the Sector Allocation Table (SAT).

# Sector Lockout Table Table (SLT)
The SLT is a contiguous segment of disk that describes the sectors on the
disk which have been locked out. The number of sectors in the SLT is
maintained in the VrD and is determined by the number of sectors on the
disk. Each entry (six bytes) in the SLT consists of two fields. The first
field is four bytes and contains the physical sector number (PSN) of the
start of a lockout range. The second field is two bytes and contains the
number of contiguous sectors to be locked out. The first zero entry
terminates the list. If no SLT exists, the PSN of the SLT in the VID is set
to zero.

# Diagnostic Test Areas (DTA)
The DTA is one sector that describes the areas on the disk available to
diagnostic programmes as read/write test areas. Each entry (six bytes) in
the DTA consists of two fields. The first field is four bytes and contains
the physical sector number (PSN) of the start of a test area. The second
field is two bytes and contains the length of the test area. The first zero
entry terminates the list. If no DTA exists, the PSN of the DTA in the VID
is set to zero.
