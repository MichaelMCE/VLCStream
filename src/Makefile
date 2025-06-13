
WIN64 := 0

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

# Graphite loop optimizations
GLOP = 

else

SYSARCH=$(X32ARCH)
RESTARGET= --target=pe-i386
BUILDM32M64 =-m32 -DWIN32=1 -D__WIN32__=1 
CONTRIBS =../../contribs/$(SYSARCH)/

# -DNDEBUG=1 -D_NDEBUG=1

#CFLAGSARCH = -march=i686 -mtune=i686 -m32 -mthreads -fstack-check -fno-common  -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium3 -mtune=pentium3 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium4 -mtune=pentium4 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=amdfam10 -mtune=amdfam10 -m32 -mthreads -fno-common -Wl,--stack,8388608
#CFLAGSARCH = -march=i686 -mtune=i686 -m32
#CFLAGSARCH = -march=native -mtune=native -fno-common
CFLAGSARCH =  

#  -Wl,--stack,18388608

#GLOP = -floop-interchange -floop-strip-mine -floop-block
GLOP = 
endif

############################################################################
############################################################################



# -Wimplicit-fallthrough=0
# -fstack-protector-all  -fdiagnostics-color=always -funsafe-loop-optimizations -Wno-enum-conversion
#PFLAGS = -Wimplicit-fallthrough=0 -ftree-vectorize -falign-functions=1 -falign-jumps=1 -falign-loops=1 -falign-labels=1 -falign-functions=16 -falign-loops=16 -fno-signed-zeros -minline-all-stringops -Wlarger-than=524288 -Wno-cpp -Wextra -Wno-error=unused-parameter -Wno-error=sign-compare -Wno-unused-parameter -Wno-sign-compare 
PFLAGS = -Wno-bool-compare
#PFLAGS = -Wfatal-errors   

OPFLAGS = 
#OPFLAGS = -mfpmath=387 -DHAVE_MMX -DUSE_MMX -mmmx
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse 
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse -DHAVE_SSE2 -DUSE_SSE2 -msse2 
#OPFLAGS = -mfpmath=sse,387 -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2 -DHAVE_MMX2 -DUSE_SSE -DHAVE_SSE -msse -DHAVE_MMX -DUSE_MMX -mmmx 
#OPFLAGS = -mfpmath=sse,387 -DUSE_3DNOW -DHAVE_3DNOW -m3dnow -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2 -DHAVE_MMX2 -DUSE_SSE -DHAVE_SSE -msse -DHAVE_MMX -DUSE_MMX -mmmx 
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2


#CFLAGS = -static -static-libstdc++ -static-libgcc $(OPFLAGS) $(BUILDM32M64) -Iinclude/ -L"include"  -I../../include/ -Isdk/include/ -I$(CONTRIBS)include/ $(PFLAGS) $(VLCFLAGS) $(GLOP) $(CFLAGSARCH) -minline-all-stringops -fno-strict-aliasing -fno-exceptions -D__MSVCRT_VERSION__=0x0701 -Wall -Werror -DPSAPI_VERSION=1 -D_WIN32 -D_M_IX86 -mwin32 -D_WIN32_WINDOWS=0x0700 -D_WIN32_IE=0x0603 -D_WIN32_WINNT=0x0601 -DWINVER=0x0801 -D__WIN32__=1 -funroll-loops -finline-functions -ftree-vectorizer-verbose=0 -fvariable-expansion-in-unroller -ftree-loop-distribution -fno-signed-zeros -fno-trapping-math -fno-math-errno -ffast-math -s -pipe -fgcse-las -fgcse-sm -fgcse-lm -fmodulo-sched -fmodulo-sched-allow-regmoves -O2 -Wno-error=format
CFLAGS = -fno-strict-aliasing $(OPFLAGS) $(BUILDM32M64) -Iinclude/ -L"include"  -I../../include/ -Isdk/include/ -I$(CONTRIBS)include/ $(PFLAGS) $(VLCFLAGS) $(GLOP) $(CFLAGSARCH) -D__MSVCRT_VERSION__=0x0701 -Wall -Werror -DPSAPI_VERSION=1 -D_WIN32 -mwin32 -D_WIN32_WINDOWS=0x0700 -D_WIN32_IE=0x0603 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D__WIN32__=1 -pipe -O3

# -fstack-protector -mwindows -fomit-frame-pointer
LIBDEPEND = -lmylcddll -lhook -lusb -lvlc.dll -lvlccore.dll -lbasswasapi -lbass 
LIBS = -static -static-libstdc++ -static-libgcc $(BUILDM32M64) -Llibs/$(SYSARCH)/ -L"lib" -L../../lib/$(SYSARCH)/ -Lsdk/lib/ -L$(CONTRIBS)lib/ $(BUILDM32M64)
# -Wl,--stack,8388608  -static -static-libstdc++ -static-libgcc
# -flto -pg -lgmon -lgcc_S  -mthreads 

# opengl/matrix.o opengl/defrc.o opengl/mylcdgl.o 
# distort.o opengl/distort/gl_distort.o opengl/distort/ripple.o 
# opengl/gl_lorenz.o opengl/gl_particles.o opengl/gl_rc.o opengl/gl_mandelbrot.o\
# -lm -lopengl32 -lglu32 




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
# BASEOBJS2 = $(BASEOBJS)


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

# -mwindows -mconsole

vlcstream.exe: $(BASEOBJS2)
	windres --output-format=coff $(RESTARGET) -i vlcstream.rc -o $(TEMPDIR)/res.o
	$(CC) $(LIBS) -o $@ $^ $(TEMPDIR)/res.o $(LIBDEPEND) -lntdll -lstdc++ -lpthread -lsetupapi -lpdh -lgdi32 -lwinmm -lpsapi -lshlwapi -ldxguid -ldinput -lole32 -lcomctl32 -loleaut32 -luuid -lgdiplus -lwininet -mwindows 
	strip.exe $@


vlcsCtrl.exe: $(TEMPDIR)/extra/vlcsCtrl.o $(TEMPDIR)/crc.o
ifeq (,$(wildcard $@))
	del /F $@ 
endif
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsCtrl.rc -o $(TEMPDIR)/resc.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/resc.o -s -lgdi32 -mwindows -m32

vlcsAddto.exe: $(TEMPDIR)/extra/vlcsAddto.o
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsAddto.rc -o $(TEMPDIR)/resa.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/resa.o -s -lgdi32 -mwindows -m32
	strip.exe $@

vlcsStart.exe: $(TEMPDIR)/extra/vlcsStart.o
	windres --output-format=coff $(RESTARGET) -i $(TEMPDIR)/extra/vlcsStart.rc -o $(TEMPDIR)/ress.o
	$(CC) -Os -o $@ $^ $(TEMPDIR)/ress.o -s -lgdi32 -mwindows -m32
	strip.exe $@


vlcsCtrl: vlcsCtrl.exe

vlcscmd: vlcsCtrl.exe

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -std=gnu18
	@echo ##

%.o: %.cpp $(DEPS)
	$(GPP) -c -o $@ $< $(CFLAGS) -DINITGUID -DUSE___UUIDOF=0
	
all : $(PROGRAMS)

vlcstream: vlcstream.exe

utils : vlcsStart.exe vlcsAddto.exe vlcsCtrl.exe

clean:
	del *.exe *.o *.bak


### PHONY define
.PHONY: xcpy all all-before all-after clean clean-custom
