VLCStream - A media player for the SwitchBlade UI using VLC v2.0.x


Install:
Get VLC 2.0.x from: http://www.videolan.org/


Unpack the Vlcstream archive in to the VLC install location.
(vlcstream.exe and vlc.exe should be in the same directory.)


Executing:
Run vlcstream.exe, drag'n'drop media on to this file or pass media path to vlcstream.eze via a Windows console prompt or shortcut.
Eg; vlcstream "f:\path\to\file.ext"
Eg; vlcstream "f:\path\to\a playlist.m3u8"

Use < and > to set per track VLC specific options.
Eg; vlcstream "dvb-t://frequency=522000000 <bandwidth=8|no-spu>"
Eg; vlcstream "f:\path\to\mediafile.ext <video-filter=rotate|rotate-angle=45>"



Controls:
To enable mouse control: Shift+Control+A or Q or Enter, or run 'vlcsEnableCursor.exe'
Middle Mouse button to end.
Right mouse gp back to previous page/screen.
Mouse control can also be ended by clicking the top left of the Playback and Home screens.


Play selected track (blue highlight): Control + Play
Stop playback: Control + Stop
Select previous track: Control + Prev
Jump to next track: Control + Next
Adjust volume up/down: Control + Volume keys
Take a Screenshot: Control + Alt + S
(keys are configurable from within vsskin/data/config.cfg)



VLCStream also provides an internal command prompt for advanced playback control and playlist manipulation.

To enable command prompt and redirect keyboard control to the LCD: Control+Shift+L or P.
Escape to close prompt and return keyboard control back to Windows.

Usage:
Commands are preceded by a '#', eg; #help
All other input without the hash (#) is processed as a search query.

Eg;
#volume 85
    Set volume to 85% (a track must be playing for this to take effect)

hands clean (
    Search through all track paths' and meta details (where and when available) for the string 'hands clean ('
>hands clean (
    As above then play if found
:hands clean (
    Search for the next instance
:>hands clean (
    As above then play if found
~hands clean (
    Reset search to beginning of current playlist
~>hands clean (
    As above then play if found
@hands clean (
    Begin search from first (root) playlist
@>hands clean (
    As above then play if found

#import c:\my music
    recursively import all tracks from the location c:\my music



Command format: #cmd argu1 argu2 etc..

Command list:
nn                                 - go to playlist or track nn (eg; '#23')
help                               - print available commands

(playlist manager)
plm getid                          - display Id of highlighted playlist (what you're looking at)
plm move fromId toId               - move playlist fromId to the position occupied by toId, moving toId to the right
plm movein fromId toId             - move playlist fromId in and to the end of toId (eg; '#plm mi 43ab 4002')
plm copy fromId dstId nodup        - copy the contents of fromId in to dstId. add 'nodup' to disable item duplication
plm setpaths Id <path>             - change the path of every track in the specified playlist to <path>. Use this when the media has moved to another location. (eg; '#plm sp 44CB o:\media\Alanis\Cant dance\')
plm new Id <name>                  - create an empty playlist called <name> in playlist Id.
plm delete Id                      - remove playlist Id, where Id is the playlist Id as denoted by '#plm getid' (eg; '#plm del 4003')
plm load Id <path or .m3u8>        - import a directory or playlist (.m3u8 only) in to the specified playlist (eg; '#plm ld 41CF O:\media\')
plm save Id <filepath.m3u8>        - generate a playlist file (.m3u8) from the specified playlist (eg; '#plm save 4001 myplaylist'}
plm Id                             - go to this playlist


(playlist)
pl                                 - open current playlist page
playlist new <name>                - create an empty playlist
playlist prune                     - remove any item which points to an invalid path from within selected playlist
playlist copy #f-t #p              - copy tracks at positions #f-t to playlist #p (f:from, t:to, p:playlist). eg; 'pl cp #9-1 #5' (use plm page to derive playlist #number)
playlist move #from #to            - move item at position #f to position #t. eg; 'pl mv #8 #1'
playlist del #f-t                  - delete itemss from #f to t. eg; '#pl del #4' and '#pl del #2-15'
playlist clear                     - remove all tracks from playlist
playlist settitle <title>          - rename title (tracks only) of selected item, if none selected, then playing track title
playlist setartist <artist>        - set artist of selected playlist item
playlist add <dir/track>           - add track or contents of a directory in to selected playlist
playlist save <path to .m3u8>      - save current playlist to <path> (m3u8 format only (utf8))
playlist load #n <path to .m3u8>   - import .m3u8 playlist <path> in to playlist #n. use #0 to import in to root
TODO: playlist remove <mtag> <tag>       - delete tracks matching meta tag <tag> from selected playlist. (eg; '#pl remove artist alanis', '#pl remove path .jpg')
playlist exclude <mtag> <tag>      - as above but generates a new playlist containing removed items
playlist extract <mtag> <tag>      - create a playlist from selected playlist with only those tracks which carry this tag
playlist decompose <mtag>          - separate selected playlist in to multiple playlists by tag (eg; #pl decom album)


shelf                              - display playlist shelf (same as #pl and #alb)
shelf left                         - scroll shelf 3 tracks from left to right
shelf right                        - scroll shelf 3 tracks from right to left
shelf prev                         - move shelf one place back
shelf next                         - move shelf one place forward
shelf first                        - scroll to beginning of shelf
shelf last                         - scroll to end of self
shelf openart                      - open cover art with the default Windows application.
shelf setart                       - copy art filepath to prompt
shelf setart <art file path>       - use this to manually set cover artwork, then use #flushart to activate.
shelf setname                      - copy name/title to prompt
shelf setname <track title>        - rename track/playlist item
shelf setalbum                     - copy album tag to prompt
shelf setalbum <album>             - set album name to <album>
shelf setpath                      - copy item path prompt
shelf setpath <path>               - set file path of selected track

dvb                                - if available, go to DVB page
dvb genepg                         - generate a separate playlist containing PIDs found in current stream.

find <mtag> <query>                - find a track by its meta information (eg; '#find album under rug')

sorta <mtag>                       - sort selected playlist by ascending order by meta tag (album, artist, path, etc..)
sortd <mtag>                       - as above but in descending order

play                               - play selected track
play #n                            - play track n
stop                               - stop playback
next                               - go to next track
prev                               - go to previous track
pause                              - pause playing track

first                              - same as '#shelf first'
last                               - same as '#shelf last'
playing                            - jump to whichever track is playing
selected                           - jump to selected track within album playlist

time h:m:s                         - jump to position in track (hh:mm:ss)
fastforward n                      - skip forward n seconds
rewind n                           - go back n seconds
random                             - on/off. enable/disable random track playback

(dvdnav: VLC 2.0 or later required)
dvdnav up                          - navigates up in the dvd menu (#dvd up)
dvdnav down                        - navigate down
dvdnav left                        - navigate left
dvdnav right                       - navigate right
dvdnav go                          - select option or feature

quit                               - exit program
end                                - close the input prompt (same as with pressing Escape)
close                              - close all pages and return to main
back                               - return to previous screen

meta                               - display the meta properties page
clock                              - show the clock
config                             - go to config page
stream                             - display general codec and available stream meta data
subtitle                           - if available, open subtitle selection dialog

title n                            - go to video track n when playing multi track video (eg; VOB)
chapter n                          - go to chapter n of playing title (eg; VOB, MKV, etc..)
chapter next                       - select next chapter
chapter previous                   - select previous chapter
nc                                 - select next chapter
pc                                 - select previous chapter
nt                                 - go to next title
pt                                 - go to previous title

eq                                 - display equalizer page
eq n                               - select equalizer preset from 0 (off) to 17

getmeta                            - try to preload all track meta data and artwork of selected playlist (#gm)
getlengths                         - try to retrieve length of all media in playlist (#gl)
flushart                           - remove all but the mot recently used artwork from memory. (#fa)
stats                              - display realtime media related codec stats (a track must be playing)

fps on                             - display a few timing stats
fps off                            - switch off

resync                             - reconnect to SBUI (can be typed blind)
about                              - print general application information
snapshot <filename>                - generate .png snapshot of current video. (#ss)

rotate <angle>                     - rotate video by n degrees. '#rotate 0' to reset
scale <factor>                     - scale video by factor (float), This does not effect aspect ratio. eg; '#scale 1.22'
scale op <n>                       - set scale method. 1:Bicubic, 2:Bilinear, 3:Nearest Neighbour
scale reset                        - disable video scaling and reset to defaults
scale                              - display current config

mouse on                           - switch on mouse (or device of this class) control, can also use left control+left shift+a or q
mouse off                          - switch off mouse control

aspect auto                        - set video aspect ratio to auto detect (highest performing and most accurate)
aspect <n:n>                       - force video aspect ratio to x:y (eg; 16:9)
aspect <n.n>                       - force ar to x.y (eg; 1.77)

rgbswap on                         - enable Red/Blue component swap of video. Sometimes required with the DShow module or media.
rgbswap off                        - disable above swap

idle on                            - allow Appl to enter idle mode after a preset period of inactivity. (default On)
idle off                           - disable idle feature

open                               - open file browser
open <dir/track>                   - import directory/track in to selected playlist
load <dir/track>                   - as above
import <dir>                       - as above but imports in to a newly created playlist with the name <dir>. Use '#shelf name' to rename playlist

volume n                           - set volume from 0 to 100 (valid only during media playback)
                                     0 = mute

(Visualizations in libVLC 2.0.x are currently unavailable)
visual                             - enable audio visualization
visual off                         - disable audio visualization
visual n                           - set which audio visual to use (eg; 3)
                                     0:none, but display available track meta  (default)
                                     1:VU meter
                                     2:Spectrometer
                                     3:Pineapple
                                     4:Spectrum
                                     5:Scope
                                     6:Goom 800x480
                                     7:Goom 400x240
                                     8:Goom 224x134
                                  for Logitech G19
                                     6:Goom 320x240
                                     7:Goom 224x168
                                     8:Goom 160x90
                                  for USBD480
                                     6:Goom 480x272
                                     7:Goom 320x180
                                     8:Goom 160x90


Meta tags are:
Title, Artist, Genre, Copyright, Album, Track, Description, Rating, Date, Setting, URL,
Language, NowPlaying, Publisher, EncodedBy, artPath, TrackID, Length, Filename, Playlist, Path


Equalizer presets are:
0:  Flat (off)
1:  Classical
2:  Club
3:  Dance
4:  Full bass
5:  Full bass treble
6:  Full treble
7:  Headphones
8:  Large hall
9:  Live
10: Party
11: Pop
12: Reggae
13: Rock
14: Ska
15: Soft
16: Soft rock
17: Techno


VLC Hotkeys:
Purpose of this page is to provide rudimentry functionality to facilitate control of VLC during its playback.
To achive this, VLc's Global Hotkeys feature is used, but will require manual configuration from within VLC.
To configure a function; Open VLC then press Control+P, navigate to the Hotkeys dialog then set a few Global hotkeys. Add the equivilent hotkey to vsskin\config.cfg

My personal defaults are configured as:
ALT+SHIFT+P: Play/Pause
ALT+SHIFT+O: Stop
ALT+SHIFT+B: Previous
ALT+SHIFT+N: Next
ALT+SHIFT+I: Volume Up
ALT+SHIFT+H: Seek Back
ALT+SHIFT+J: Seek Forward
ALT+SHIFT+F: Fullscreen
ALT+SHIFT+M: Mute
ALT+SHIFT+K: Volume Down
ALT+SHIFT+Q: Close VLC (will not close VLCStream)

VLCStream Hotkeys are configured through vsskin\config.cfg, which may be edited with notepad.exe.

As this is implemented as nothing more than a general, but configurable, hotkey simulator, it may be used to control any global hotkey supporting application.

---------------


Note regarding the artwork:
libVLC will first search the media folder for an artwork file named 'cover.jpg', if this exists then it's used, otherwise libVC will attempt to download track artwork from the web.
Artwork is then available to VlcStream upon download completion.

The speed at which VLC acquires artwork depends on several factors, but mostly related to track meta tags, internet bandwidth and availability of search provider.
Once VlcStream has been passed then image location it's saved in the playlist for later and speedier retrieval.

Playlist file 'vlcsplaylist.m3u8' is used to store the current playlist between sessions. This file is generated at program exit and will overwrite previous contents.






Feedback and suggestions welcomed.
----------------------------------

Michael McElligott
web: http://mylcd.sourceforge.net

Contact:
Email: okio@users.sourceforge.net
http://mylcd.sourceforge.net/vlc/index.html

----------------------------------





