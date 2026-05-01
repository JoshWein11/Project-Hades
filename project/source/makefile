# Compiler to use
CC = gcc
AR = ar

# Compiler flags
CFLAGS = -Wall -Iinclude

# Linker flags (link our new setting lib, then raylib, then windows libs)
LDFLAGS = -Llib -laudio -lchar -lsetting -lweapon -ltiled -lenemy -lsavedata -lpuzzle -lboss -lraylib -lopengl32 -lgdi32 -lwinmm

# Target executable
TARGET = game.exe

# Files
MAIN_OBJ = main.o

# Modules
SCREENS_OBJ = module/screen_splash.o module/screen_menu.o module/screen_difficulty.o module/screen_dialogue.o module/screen_gameplay.o module/screen_setting.o module/screen_credits.o
SETTING_OBJ = module/setting.o
SETTING_LIB = lib/libsetting.a
CHAR_OBJ = module/character.o
CHAR_LIB = lib/libchar.a
TILED_OBJ = module/tiled_map.o
TILED_LIB = lib/libtiled.a
WEAPON_OBJ = module/weapon.o
WEAPON_LIB = lib/libweapon.a
AUDIO_OBJ = module/audio.o
AUDIO_LIB = lib/libaudio.a
ENEMY_OBJ = module/enemy.o
ENEMY_LIB = lib/libenemy.a
SAVEDATA_OBJ = module/savedata.o
SAVEDATA_LIB = lib/libsavedata.a
PUZZLE_OBJ = module/puzzle.o
PUZZLE_LIB = lib/libpuzzle.a
BOSS_OBJ = module/boss.o
BOSS_LIB = lib/libboss.a


# Default rule
all: $(TARGET)

# Link the final executable using the main object and our newly built library
$(TARGET): $(MAIN_OBJ) $(SCREENS_OBJ) $(SETTING_LIB) $(CHAR_LIB) $(TILED_LIB) $(WEAPON_LIB) $(AUDIO_LIB) $(ENEMY_LIB) $(SAVEDATA_LIB) $(PUZZLE_LIB) $(BOSS_LIB)
	$(CC) $(MAIN_OBJ) $(SCREENS_OBJ) -o $(TARGET) $(CFLAGS) $(LDFLAGS) 

# Create the static library for the setting module
$(SETTING_LIB): $(SETTING_OBJ)
	$(AR) rcs $@ $^

$(CHAR_LIB): $(CHAR_OBJ)
	$(AR) rcs $@ $^

$(TILED_LIB): $(TILED_OBJ)
	$(AR) rcs $@ $^

$(WEAPON_LIB): $(WEAPON_OBJ)
	$(AR) rcs $@ $^

$(AUDIO_LIB): $(AUDIO_OBJ)
	$(AR) rcs $@ $^

$(ENEMY_LIB): $(ENEMY_OBJ)
	$(AR) rcs $@ $^

$(SAVEDATA_LIB): $(SAVEDATA_OBJ)
	$(AR) rcs $@ $^

$(PUZZLE_LIB): $(PUZZLE_OBJ)
	$(AR) rcs $@ $^

$(BOSS_LIB): $(BOSS_OBJ)
	$(AR) rcs $@ $^

# Generic rule to compile any .c file into a .o object file
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to clean up compiled files (Windows commands)
clean:
	del /Q $(TARGET) $(MAIN_OBJ) $(SCREENS_OBJ) module\setting.o lib\libsetting.a module\character.o lib\libchar.a module\tiled_map.o lib\libtiled.a module\weapon.o lib\libweapon.a module\audio.o lib\libaudio.a module\enemy.o lib\libenemy.a module\savedata.o lib\libsavedata.a module\puzzle.o lib\libpuzzle.a
