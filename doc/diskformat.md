= Disk structures =

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

DB
: Data Block / block of sequential records. Variable length.

FILE
: Linear file 

HDR
: Header (first few bytes of each SDB, PDB, or FAB); contains linkage
information to next structure of same type.  FABs have forward and
backward links; other structures have only forward links.

SLT
: Sector Lockout Table.

DTA
: Diagnostic Test Areas.

PSN
: Pysical Sector Number

= Overview =

'''
(block 0)
/-------\    /-------\
|  VID  |--->|  SAT  |
\-------/    \-------/
    |
    v
/-------\    /-------\ 
|  SDB  |--->|  SDB  | ...
|-------|    \-------/  
| SDE.. |
\-------/
    |
    v
/-------\    /-------\
|  PDB  |--->|  PDB  | ...
|-------|    \-------/  
| PDE.. |
\-------/\
    |     \  /---------\
    |      ->| FILE    |
    | 
    V  
/-------\    /-------\
|  FAB  |--->|  FAB  | ...
|-------|    \-------/  
    | 
    |
    V
/-------\
| DB..  |
\-------/
'''

= Volume ID Block (VID) =

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

= Sector Allocation Table (SAT) =

Sector Allocation Table / Map. Variable length.

One bit per sector of the disk.

= Secondary Directory Block (SDB) =

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

= Primary Directory Block (PDB) =

One catalogue of files. Each 4 sectors large with room for up to NN files,
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

Each PDE contains the information about one file name


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

= File Access Block (FAB) ==

TBD (Page 84)


