



APP_TYPE = -mwindows
#APP_TYPE = -mconsole

CC=gcc
GPP=g++
X64ARCH=x64
X32ARCH=x32
VLCFLAGS= -DMODULE_STRING="\"VLCStream\""
PROGRAMS=vlcstream.exe vlcsStart.exe vlcsAddto.exe vlcsCtrl.exe
TEMPDIR=M:



############################################################################
############################################################################

ifeq ($(WIN64),1)
SYSARCH=$(X64ARCH)
RESTARGET= 
BUILDM32M64 =-m64 -DWIN64=1 -D__WIN64__=1 -DHAVE_STRUCT_TIMESPEC -D_CRT_NON_CONFORMING_SWPRINTFS -D__USE_MINGW_ANSI_STDIO
CONTRIBS =../../contribs/$(SYSARCH)/
CFLAGSARCH =-march=x86-64 -mtune=generic -m64

else

SYSARCH=$(X32ARCH)
RESTARGET= --target=pe-i386
BUILDM32M64 =-m32 -DWIN32=1 -D__WIN32__=1 
CONTRIBS =../../contribs/$(SYSARCH)/

#CFLAGSARCH = -march=i686 -mtune=i686 -m32 -mthreads -fstack-check -fno-common  -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium3 -mtune=pentium3 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium4 -mtune=pentium4 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=amdfam10 -mtune=amdfam10 -m32 -mthreads -fno-common -Wl,--stack,8388608
#CFLAGSARCH = -march=i686 -mtune=i686 -m32
#CFLAGSARCH = -march=native -mtune=native -fno-common
CFLAGSARCH =  

endif

############################################################################
############################################################################



SDK3220 = sdk32_2.0
SDK3222 = sdk32_2.2
SDK6430 = sdk64_3.0.0
SDK6419 = sdk64_3.0.19


ifeq ($(SDK),0)
# be silent

else ifeq ($(SDK),3220)
VLCSDK = sdk/$(SDK3220)
$(info VLC SDK: x32 v2.0.0)

else ifeq ($(SDK),3222)
VLCSDK = sdk/$(SDK3222)
$(info VLC SDK: x32 v2.2.0)

else ifeq ($(SDK),6430)
VLCSDK = sdk/$(SDK6430)
$(info VLC SDK: x64 v3.0.0)

else ifeq ($(SDK),6419)
VLCSDK = sdk/$(SDK6419)
$(info VLC SDK: x64 v3.0.19)

else 
# set default
VLCSDK = sdk/$(SDK3222)
$(info VLC SDK: x32 v2.2.0)

endif


PFLAGS = -Wno-bool-compare
OPFLAGS = 
#OPFLAGS = -mfpmath=387 -DHAVE_MMX -DUSE_MMX -mmmx
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse 

CFLAGS = -fno-strict-aliasing $(OPFLAGS) $(BUILDM32M64) -Iinclude/ -L"include"  -I../../include/ -I$(VLCSDK)/include/ -I$(CONTRIBS)include/ $(PFLAGS) $(VLCFLAGS) $(CFLAGSARCH) -D__MSVCRT_VERSION__=0x0701 -Wall -Werror -DPSAPI_VERSION=1 -D_WIN32 -mwin32 -D_WIN32_WINDOWS=0x0700 -D_WIN32_IE=0x0603 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D__WIN32__=1 -O3 -std=gnu18
CPPFLAGS = $(OPFLAGS) $(BUILDM32M64) -Iinclude/ -L"include"  -I../../include/ -I$(VLCSDK)/include/ -I$(CONTRIBS)include/ $(PFLAGS) $(VLCFLAGS) $(CFLAGSARCH) -Wall -Werror -DPSAPI_VERSION=1 -D_WIN32 -mwin32 -DINITGUID -DUSE___UUIDOF=0 -O3 -std=c++17

LIBSOWN    = -lmylcddll -lhook -lusb -lvlc.dll -lvlccore.dll -lbasswasapi -lbass 
LIBSSYSTEM = -lntdll -lstdc++ -lpthread -lsetupapi -lpdh -lgdi32 -lwinmm -lpsapi -lshlwapi -ldxguid -ldinput -lole32 -lcomctl32 -loleaut32 -luuid -lgdiplus -lwininet
LIBDEPEND  = $(LIBSOWN) $(LIBSSYSTEM) 

LIBS = -static -static-libstdc++ -static-libgcc $(BUILDM32M64) -Llibs/$(SYSARCH)/ -L"lib" -L../../lib/$(SYSARCH)/ -L$(VLCSDK)/lib/ -L$(CONTRIBS)lib/ $(BUILDM32M64)


BASEOBJS =  tcx.o fileext.o process.o fileal.o hdtouch.o bass.o imagec.o settings.o\
			winm.o tags.o playlistc.o drawvolume.o stringcache.o vlceventcb.o list.o\
			playlistsort.o playlistmanager.o stack.o m3u.o libmylcd.o ext.o fileio.o\
			cmdparser.o vaudio.o common.o funstuff.o vlcstream.o marquee.o\
			vlc.o cpu.o lock.o transform.o libvlceq.o crc.o input.o editbox.o\
			config.o timer.o swatch.o memory.o artc.o picqueue.o page.o playlist.o\
			filebrowser.o tree.o shelf.o sbui.o\
			tetris/twrap.o tetris/stc.o\
			antplus/anthrm.o antplus/garminhr.o antplus/libantplus.o\
			cc/listbox.o cc/cc.o cc/panel.o cc/scrollbar.o cc/tv.o cc/pane.o cc/lb.o\
			cc/keypad.o cc/graph.o cc/slider.o cc/button2.o cc/button.o cc/label.o\
			mxml/mxml-index.o mxml/mxml-node.o mxml/mxml-entity.o mxml/mxml-file.o\
			mxml/mxml-attr.o mxml/mxml-search.o mxml/mxml-set.o mxml/mxml-string.o\
			mxml/mxml-private.o\
			pages/page_alarm.o        pages/page_album.o     pages/page_antplus.o       \
			pages/page_chapter.o      pages/page_clock.o     pages/page_ctrloverlay.o   \
			pages/page_epg.o          pages/page_eq.o        pages/page_es.o            \
			pages/page_exit.o         pages/page_exppanel.o  pages/page_filepane.o      \
			pages/page_garmin.o       pages/page_home.o      pages/page_imgovr.o        \
			pages/page_imgpane.o      pages/page_keyboard.o  pages/page_mediastats.o    \
			pages/page_meta.o         pages/page_player.o    pages/page_plypane.o       \
			pages/page_plypanel.o     pages/page_plytree.o   pages/page_search.o        \
			pages/page_sub.o          pages/page_taskman.o   pages/page_tetris.o        \
			pages/page_textoverlay.o  pages/page_cfg.o       pages/page_videotransform.o\
			pages/page_plm.o          pages/page_hotkeys.o   pages/page_video.o


BASEOBJS2 = $(BASEOBJS:%=$(TEMPDIR)/%)


test: 
	@echo $(BASEOBJS2)

xcpy:
	@xcopy *.c $(TEMPDIR)\ /y /d /q /I > NUL
	@xcopy *.cpp $(TEMPDIR)\ /y /d /q /I > NUL
	@xcopy *.h $(TEMPDIR)\ /y /d /q /I > NUL
	@xcopy sdk "$(TEMPDIR)\sdk" /y /d /q /I /S > NUL
	@xcopy bass "$(TEMPDIR)\bass" /y /d /q /I > NUL
	@xcopy hook "$(TEMPDIR)\hook" /y /d /q /I > NUL
	@xcopy extra "$(TEMPDIR)\extra" /y /d /q /I > NUL
	@xcopy cc "$(TEMPDIR)\cc" /y /d /q /I > NUL
	@xcopy tetris "$(TEMPDIR)\tetris" /y /d /q /I > NUL
	@xcopy antplus "$(TEMPDIR)\antplus" /y /d /q /I > NUL
	@xcopy mxml "$(TEMPDIR)\mxml" /y /d /q /I > NUL
	@xcopy pages "$(TEMPDIR)\pages" /y /d /q /I > NUL
	@xcopy cleanobjs.bat $(TEMPDIR)\ /y /d /q /I > NUL

vlcstream.exe: $(BASEOBJS2)
	$(info   )
	windres --output-format=coff $(RESTARGET) -i vlcstream.rc -o $(TEMPDIR)/res.o
	$(CC) $(LIBS) -o $@ $^ $(TEMPDIR)/res.o $(LIBDEPEND) $(APP_TYPE)
	strip.exe $@


cleanextra:
	del /F "$(TEMPDIR)/crc.o"
	del /F "$(TEMPDIR)/resa.o"
	del /F "$(TEMPDIR)/ress.o"
	del /F "$(TEMPDIR)/resc.o"
	
vlcsCtrl.exe: $(TEMPDIR)/extra/vlcsCtrl.o $(TEMPDIR)/crc.o
ifeq (,$(wildcard $@))
	del /F $@ 
endif
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsCtrl.rc -o $(TEMPDIR)/resc.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/resc.o -s -lgdi32 $(APP_TYPE) $(BUILDM32M64)

vlcsAddto.exe: $(TEMPDIR)/extra/vlcsAddto.o
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsAddto.rc -o $(TEMPDIR)/resa.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/resa.o -s -lgdi32 $(APP_TYPE) $(BUILDM32M64)
	strip.exe $@

vlcsStart.exe: $(TEMPDIR)/extra/vlcsStart.o
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsStart.rc -o $(TEMPDIR)/ress.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/ress.o -s -lgdi32 $(APP_TYPE) $(BUILDM32M64)
	strip.exe $@


vlcsCtrl: vlcsCtrl.exe

vlcscmd: vlcsCtrl.exe

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	$(info   )

%.o: %.cpp $(DEPS)
	$(GPP) -c -o $@ $< $(CPPFLAGS)
	$(info   )

all : $(PROGRAMS)

vlcstream: vlcstream.exe

utils : vlcsStart.exe vlcsAddto.exe vlcsCtrl.exe
extra : utils

clean:
	del *.exe *.o *.bak


### PHONY define
.PHONY: xcpy all all-before all-after clean clean-custom
