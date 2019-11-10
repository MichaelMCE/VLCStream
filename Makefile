
WIN64 := 0

CC=gcc
GPP=g++
X64ARCH=x64
X32ARCH=x32
VLCFLAGS= -DMODULE_STRING="\"VLCStream\""
PROGRAMS=vlcstream.exe vlcsStart.exe vlcsAddto.exe vlcsCtrl.exe



############################################################################
############################################################################

ifeq ($(WIN64),1)
SYSARCH=$(X64ARCH)
RESTARGET= 
BUILDM32M64 =-m64 -DWIN64=1 -D__WIN64__=1 -DHAVE_STRUCT_TIMESPEC
CONTRIBS =../../contribs/$(SYSARCH)/
CFLAGSARCH =-march=k8 -mtune=k8 -m64

# Graphite loop optimizations
GLOP = 

else

SYSARCH=$(X32ARCH)
RESTARGET= --target=pe-i386
BUILDM32M64 =-m32 -DWIN32=1 -D__WIN32__=1 -DNDEBUG=1 -D_NDEBUG=1
CONTRIBS =../../contribs/$(SYSARCH)/

#CFLAGSARCH = -march=i686 -mtune=i686 -m32 -mthreads -fstack-check -fno-common  -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium3 -mtune=pentium3 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=pentium4 -mtune=pentium4 -m32 -mthreads -Wl,--stack,8388608
#CFLAGSARCH = -march=amdfam10 -mtune=amdfam10 -m32 -mthreads -fno-common -Wl,--stack,8388608
CFLAGSARCH = -march=i686 -mtune=i686 -fno-common
#  -Wl,--stack,18388608

GLOP = -floop-interchange -floop-strip-mine -floop-block
#GLOP = 
endif

############################################################################
############################################################################



# -fstack-protector-all  -fdiagnostics-color=always -funsafe-loop-optimizations 
PFLAGS = -ftree-vectorize -falign-functions=1 -falign-jumps=1 -falign-loops=1 -falign-labels=1 -falign-functions=16 -falign-loops=16 -fno-signed-zeros -minline-all-stringops -Wlarger-than=262144 -Wno-cpp -Wextra -Wno-error=unused-parameter -Wno-error=sign-compare -Wno-unused-parameter -Wno-sign-compare
#PFLAGS = -Wfatal-errors  -Wno-bool-compare

#OPFLAGS = -mfpmath=387 -DHAVE_MMX -DUSE_MMX -mmmx
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse 
#OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse -DHAVE_SSE2 -DUSE_SSE2 -msse2 
#OPFLAGS = -mfpmath=sse,387 -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2 -DHAVE_MMX2 -DUSE_SSE -DHAVE_SSE -msse -DHAVE_MMX -DUSE_MMX -mmmx 
#OPFLAGS = -mfpmath=sse,387 -DUSE_3DNOW -DHAVE_3DNOW -m3dnow -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2 -DHAVE_MMX2 -DUSE_SSE -DHAVE_SSE -msse -DHAVE_MMX -DUSE_MMX -mmmx 
OPFLAGS = -mfpmath=sse,387 -DHAVE_MMX -DUSE_MMX -mmmx -DUSE_SSE -DHAVE_SSE -msse -DHAVE_SSE2 -DUSE_SSE2 -msse2 -DUSE_MMX2


CFLAGS = -static-libgcc  $(OPFLAGS) $(BUILDM32M64) -Iinclude/ -L"include"  -I../../include/ -Isdk/include/ -I$(CONTRIBS)include/ $(PFLAGS) $(VLCFLAGS) $(GLOP) $(CFLAGSARCH) -minline-all-stringops -fno-strict-aliasing -fno-exceptions -D__MSVCRT_VERSION__=0x0701 -Wall -Werror -DPSAPI_VERSION=1 -D_WIN32 -D_M_IX86 -mwin32 -D_WIN32_WINDOWS=0x0700 -D_WIN32_IE=0x0603 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -D__WIN32__=1 -funroll-loops -finline-functions -ftree-vectorizer-verbose=0 -fvariable-expansion-in-unroller -ftree-loop-distribution -fno-signed-zeros -fno-trapping-math -fno-math-errno -ffast-math -s -pipe -fgcse-las -fgcse-sm -fgcse-lm -fmodulo-sched -fmodulo-sched-allow-regmoves -O2 -Wno-error=format

# -static-libstdc++

# -fstack-protector -mwindows -fomit-frame-pointer
LIBDEPEND = -lmylcddll hook/$(SYSARCH)/libhook.a -lusb -lvlc.dll -lvlccore.dll bass/$(SYSARCH)/basswasapi.lib bass/$(SYSARCH)/bass.lib
LIBS = -static-libgcc -lgcc_S $(BUILDM32M64) -Llibs/$(SYSARCH)/ -L"lib" -L../../lib/$(SYSARCH)/ -Lsdk/lib/ -L$(CONTRIBS)lib/ $(BUILDM32M64) -mthreads
# -Wl,--stack,8388608 
# -flto -mwindows -pg -lgmon 

# opengl/matrix.o opengl/defrc.o opengl/mylcdgl.o 
# distort.o opengl/distort/gl_distort.o opengl/distort/ripple.o 
# opengl/gl_lorenz.o opengl/gl_particles.o opengl/gl_rc.o opengl/gl_mandelbrot.o\
# -lm -lopengl32 -lglu32 




BASEOBJS =  garmin.o tcx.o taskman.o cc/listbox.o search.o fileext.o alarm.o process.o\
			cc/cc.o cc/panel.o cc/scrollbar.o filebrowser.o cc/tv.o tree.o plytv.o\
			cfg.o sub.o imagec.o imgovr.o chapter.o meta.o exit.o bass.o antplus.o\
			winm.o tags.o playlistc.o drawvolume.o stringcache.o vlceventcb.o list.o\
			fileal.o playlistsort.o playlistmanager.o ctrloverlay.o videotransform.o\
			cmdparser.o shelf.o common.o libmylcd.o stack.o plypane.o cc/pane.o m3u.o\
			antplus/anthrm.o antplus/garminhr.o antplus/libantplus.o playlist.o ext.o\
			home.o fileio.o sbui.o eq.o mediastats.o cc/button.o cc/label.o cc/lb.o\
			plm.o vlc.o cpu.o lock.o transform.o libvlceq.o filepane.o imgpane.o crc.o\
			tetris.o tetris/twrap.o tetris/stc.o epg.o es.o album.o page.o editbox.o\
			config.o exppanel.o timer.o swatch.o memory.o plypanel.o artc.o picqueue.o\
			input.o marquee.o settings.o hotkeys.o vlcstream.o cc/slider.o cc/button2.o\
			funstuff.o clock.o cc/keypad.o keyboard.o vaudio.o cc/graph.o textoverlay.o\
			mxml/mxml-index.o mxml/mxml-node.o mxml/mxml-entity.o mxml/mxml-file.o\
			mxml/mxml-attr.o mxml/mxml-search.o mxml/mxml-set.o mxml/mxml-string.o\
			mxml/mxml-private.o\

BASEOBJS2 = $(BASEOBJS:%=M:/%)


test: 
	@echo $(BASEOBJS2)

xcpy:
	@xcopy *.c M:\ /y /d /q /I > NUL
	@xcopy *.cpp M:\ /y /d /q /I > NUL
	@xcopy *.h M:\ /y /d /q /I > NUL
	@xcopy sdk M:\sdk /y /d /q /I /S > NUL
	@xcopy bass M:\bass /y /d /q /I > NUL
	@xcopy hook M:\hook /y /d /q /I > NUL
	@xcopy cc M:\cc /y /d /q /I > NUL
	@xcopy tetris M:\tetris /y /d /q /I > NUL
	@xcopy antplus M:\antplus /y /d /q /I > NUL
	@xcopy mxml M:\mxml /y /d /q /I > NUL
	@xcopy cleanobjs.bat M:\ /y /d /q /I > NUL

# -mwindows

vlcstream.exe: $(BASEOBJS2)
	windres --output-format=coff $(RESTARGET) -i vlcstream.rc -o M:/res.o
	$(CC) $(LIBS) -o $@ $^ M:/res.o $(LIBDEPEND) -lntdll -lstdc++ -lpthread -lsetupapi -lpdh -lgdi32 -lwinmm -lpsapi -lshlwapi -ldxguid -ldinput -lole32 -lcomctl32 -loleaut32 -luuid -lgdiplus -lwininet  -mwindows
	strip.exe $@


#-lmsvcr100 

vlcsCtrl.exe: vlcsCtrl.o crc.o
	windres --output-format=coff $(RESTARGET) -i vlcsCtrl.rc -o resc.o
	$(CC) -m32 -Os -o $@ $^ resc.o -s -lgdi32 -mwindows


vlcsCtrl: vlcsCtrl.exe

vlcscmd: vlcsCtrl.exe
	

vlcsAddto.exe: vlcsAddto.o
	$(CC) -m32 -Os -o $@ $^ -mwindows 
	strip.exe $@

vlcsStart.exe: vlcsStart.o
	$(CC) -m32 -Os -o $@ $^ -mwindows
	strip.exe $@
	
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -std=gnu11

%.o: %.cpp $(DEPS)
	$(GPP) -c -o $@ $< $(CFLAGS) -DINITGUID -DUSE___UUIDOF=0
	
all : $(PROGRAMS)

vlcstream: vlcstream.exe

utils : vlcsStart.exe vlcsAddto.exe vlcsCtrl.exe

clean:
	del *.exe *.o *.bak


### PHONY define
.PHONY: xcpy all all-before all-after clean clean-custom
