qtlibs := $(strip QtCore QtGui QtWidgets QtNetwork $(if $(findstring osx,$(platform)),QtOpenGL))
include $(nall)/qt/Makefile

ui_objects := ui-main ui-base ui-cartridge ui-debugger ui-input ui-movie ui-settings ui-state ui-tools
ui_objects += ruby
ui_objects += $(if $(call streq,$(platform),$(filter $(platform),win msys)),resource)
link += $(qtlib)

headers := $(call rwildcard,$(ui)/,%.hpp)
moc_headers := $(call rwildcard,$(nall)/qt/,%.moc.hpp) $(call rwildcard,$(ui)/,%.moc.hpp)
moc_objects := $(foreach f,$(moc_headers),obj/$(notdir $(patsubst %.moc.hpp,%.moc,$f)))
qt_compile = $(call compile,-Iobj $(qtinc))

# platform
ifeq ($(platform),x)
  ruby := video.glx video.xv video.qtraster video.sdl
  ruby += audio.alsa audio.openal audio.oss audio.pulseaudio audio.pulseaudiosimple audio.ao
  ruby += input.sdl input.x

  link += $(if $(findstring audio.openal,$(ruby)),-lopenal)
else ifeq ($(platform),osx)
  ruby := video.qtopengl video.qtraster
  ruby += audio.openal
  ruby += input.macos

  link += $(if $(findstring audio.openal,$(ruby)),-framework OpenAL)
else ifeq ($(platform),$(filter $(platform),win msys))
  ruby := video.direct3d video.wgl video.directdraw video.gdi video.qtraster
  ruby += audio.directsound audio.xaudio2
  ruby += input.rawinput input.directinput

  link += $(if $(findstring audio.openal,$(ruby)),-lopenal32)
else
  unknown_platform: help;
endif

# ruby
rubyflags := $(if $(findstring .sdl,$(ruby)),`sdl-config --cflags`)
rubyflags += $(if $(findstring .qt,$(ruby)),$(qtinc))

# gross hack because at some point gcc optimization started breaking rawinput
ifeq ($(platform),$(filter $(platform),win msys))
  rubyflags += -O0
endif

link += $(if $(findstring .sdl,$(ruby)),`sdl-config --libs`)
link += $(if $(findstring video.direct3d,$(ruby)),-ld3d9)
link += $(if $(findstring video.directdraw,$(ruby)),-lddraw)
link += $(if $(findstring video.glx,$(ruby)),-lGL)
link += $(if $(findstring video.wgl,$(ruby)),-lopengl32)
link += $(if $(findstring video.xv,$(ruby)),-lXv)
link += $(if $(findstring audio.alsa,$(ruby)),-lasound)
link += $(if $(findstring audio.ao,$(ruby)),-lao)
link += $(if $(findstring audio.directsound,$(ruby)),-ldsound)
link += $(if $(findstring audio.pulseaudio,$(ruby)),-lpulse)
link += $(if $(findstring audio.pulseaudiosimple,$(ruby)),-lpulse-simple)
link += $(if $(findstring input.directinput,$(ruby)),-ldinput8 -ldxguid)
link += $(if $(findstring input.rawinput,$(ruby)),-ldinput8 -ldxguid)

rubydef := $(foreach c,$(subst .,_,$(call strupper,$(ruby))),-D$c)

# rules
objects := $(ui_objects) $(objects)

# automatically run moc on all .moc.hpp (MOC header) files
%.moc: $<; $(moc) -i $< -o $@

# automatically generate %.moc build rules
__list = $(moc_headers)
$(foreach f,$(moc_objects), \
  $(eval __file = $(word 1,$(__list))) \
  $(eval __list = $(wordlist 2,$(words $(__list)),$(__list))) \
  $(eval $f: $(__file)) \
)

$(objdir)/ui-main.o: $(ui)/main.cpp $(headers) obj/resource.rcc $(wildcard $(ui)/*.cpp) $(wildcard $(ui)/application/*.cpp) $(wildcard $(ui)/link/*.cpp) $(wildcard $(ui)/platform/*.cpp) $(wildcard $(ui)/utility/*.cpp) $(wildcard $(ui)/nwaccess/*.cpp); $(qt_compile)
$(objdir)/ui-base.o: $(ui)/base/base.cpp $(headers) $(wildcard $(ui)/base/*.cpp); $(qt_compile)
$(objdir)/ui-cartridge.o: $(ui)/cartridge/cartridge.cpp $(headers) $(wildcard $(ui)/cartridge/*.cpp); $(qt_compile)
$(objdir)/ui-debugger.o: $(ui)/debugger/debugger.cpp $(headers) $(call rwildcard,$(ui)/debugger/,%.cpp); $(qt_compile)
$(objdir)/ui-input.o: $(ui)/input/input.cpp $(headers) $(wildcard $(ui)/input/*.cpp); $(qt_compile)
$(objdir)/ui-movie.o: $(ui)/movie/movie.cpp $(headers) $(wildcard $(ui)/movie/*.cpp); $(qt_compile)
$(objdir)/ui-settings.o: $(ui)/settings/settings.cpp $(headers) $(wildcard $(ui)/settings/*.cpp); $(qt_compile)
$(objdir)/ui-state.o: $(ui)/state/state.cpp $(headers) $(wildcard $(ui)/state/*.cpp); $(qt_compile)
$(objdir)/ui-tools.o: $(ui)/tools/tools.cpp $(headers) $(wildcard $(ui)/tools/*.cpp); $(qt_compile)

$(objdir)/ruby.o: ruby/ruby.cpp $(call rwildcard,ruby/*)
	$(call compile,$(rubydef) $(rubyflags))

obj/resource.rcc: $(ui)/resource/resource.qrc $(ui)/data/*
	$(rcc) $(ui)/resource/resource.qrc -o obj/resource.rcc

$(objdir)/resource.o: $(ui)/resource/resource.rc
	windres $(ui)/resource/resource.rc $(objdir)/resource.o

# targets
ui_build: obj/resource.rcc $(moc_objects);
ui_clean:
	-@$(call delete,obj/*.rcc)
	-@$(call delete,obj/*.moc)
