# Compiler to use
CC = gcc
AR = ar

# Compiler flags
CFLAGS = -Wall -Iinclude

# Linker flags (link our new setting lib, then raylib, then windows libs)
LDFLAGS = -Llib -lchar -lsetting -lweapon -ltiled -lraylib -lopengl32 -lgdi32 -lwinmm

# Target executable
TARGET = game.exe

# Files
MAIN_OBJ = main.o

# Modules
SCREENS_OBJ = module/screen_splash.o module/screen_menu.o module/screen_dialogue.o module/screen_gameplay.o
SETTING_OBJ = module/setting.o
SETTING_LIB = lib/libsetting.a
CHAR_OBJ = module/character.o
CHAR_LIB = lib/libchar.a
TILED_OBJ = module/tiled_map.o
TILED_LIB = lib/libtiled.a
WEAPON_OBJ = module/weapon.o
WEAPON_LIB = lib/libweapon.a


# Default rule
all: $(TARGET)

# Link the final executable using the main object and our newly built library
$(TARGET): $(MAIN_OBJ) $(SCREENS_OBJ) $(SETTING_LIB) $(CHAR_LIB) $(TILED_LIB) $(WEAPON_LIB)
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

# Generic rule to compile any .c file into a .o object file
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to clean up compiled files (Windows commands)
clean:
	del /Q $(TARGET) $(MAIN_OBJ) $(SCREENS_OBJ) module\setting.o lib\libsetting.a module\character.o lib\libchar.a module\tiled_map.o lib\libtiled.a module\weapon.o lib\libweapon.a
