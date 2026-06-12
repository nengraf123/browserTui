# ==============================================================================
# НАСТРОЙКИ СБОРКИ И ПОТОКОВ
# ==============================================================================
# Автоматический запуск в 12 потоков
MAKEFLAGS += -j12

# Поиск всех файлов .cpp в папке src и её подпапках
SRCS := $(shell find src -name "*.cpp")

# Трансляция путей .cpp в .o (src/main.cpp -> src/main.o)
OBJS := $(SRCS:.cpp=.o)

# Название итогового бинарного файла
TARGET := ./bin/app

# ==============================================================================
# ФЛАГИ КОМПИЛЯЦИИ (ОПТИМИЗАЦИЯ И ОТЛАДКА)
# ==============================================================================
# Флаги для максимального уменьшения размера бинарника
SMALL_FLAGS := -Os -s -flto -march=native \
               -ffunction-sections -fdata-sections \
               -fno-rtti -fno-exceptions \
               -Wl,--gc-sections -Wl,--strip-all

# Флаги для быстрой отладки
FAST_FLAGS  := -O0 -g -pipe -Wall -Wextra

# Активные флаги компиляции
CXXFLAGS    := $(SMALL_FLAGS) -lsystemd -lgumbo -lcurl

# ==============================================================================
# КОМАНДЫ ЗАПУСКА ПРИЛОЖЕНИЯ
# ==============================================================================
START_NORMAL := $(TARGET)
START_KITTY  := kitty --class fullscreen -e $(TARGET)
START_FLOAT  := kitty --class float \
                -o remember_window_size=no \
                -o initial_window_width=42c \
                -o initial_window_height=21c \
                -e $(TARGET)

# Выбранный способ запуска по умолчанию
LAUNCH       := $(START_NORMAL)

# ==============================================================================
# ПРАВИЛА СБОРКИ (ЦЕЛИ)
# ==============================================================================
.PHONY: all run clean

# Главная цель: компиляция проекта, запуск и последующая очистка
all: $(TARGET)
	$(LAUNCH)
	$(MAKE) clean

# Сборка финального бинарника из объектных файлов и main.cpp
$(TARGET): $(OBJS) main.cpp
	g++ main.cpp $(OBJS) $(CXXFLAGS) -o $(TARGET)

# Шаблонное правило: компиляция каждого .cpp в .o (выполняется параллельно)
%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

# Просто запуск уже собранного приложения
run:
	$(LAUNCH)

# Очистка объектных файлов
clean:
	rm -f $(OBJS)

# # Автоматически запускает make в 12 потоков, даже если вы вызвали просто :!make
# MAKEFLAGS += -j12
# # Находим все файлы автоматически (включая новые, если появятся)
# # SRCS := $(wildcard src/*.cpp) $(wildcard src/APP/*.cpp) $(wildcard src/APP/**.cpp)
# SRCS := $(shell find src -name "*.cpp")
# # Превращаем пути .cpp в пути .o (например, src/main.o)
# OBJS := $(SRCS:.cpp=.o)
#
#
# start1 := ./bin/app
# start2 := kitty --class fullscreen -e ./bin/app
# start3 := kitty --class float -o remember_window_size=no -o initial_window_width=42c -o initial_window_height=21c -e ./bin/app
#
# smallCom := -Os -s -flto -march=native \
# 	-ffunction-sections -fdata-sections \
# 	-fno-rtti -fno-exceptions \
# 	-Wl,--gc-sections -Wl,--strip-all
#
# fastCom := -O0 -g -pipe -Wall -Wextra
#
# # Добавляем флаги компиляции
# CXXFLAGS := $(smallCom)
# Com := 
# Для_Скриншотилки := -lsystemd
#
# # Главная цель сначала собирает исполняемый файл, затем запускает
# all: 
# 	$(com)
# 	$(start1)
# 	${clean}
#
# # Правило для сборки финального бинарника из объектных файлов
# com:= $(OBJS) \
# 	g++ main.cpp $(OBJS) $(smallCom) -lcurl -o bin/app $(Для_Скриншотилки)
#
# # Шаблонное правило: как скомпилировать ОДИН .cpp в ОДИН .o
# # Именно это правило make будет выполнять параллельно в 12 потоков!
# %.o: %.cpp
# 	g++ $(CXXFLAGS) -c $< -o $@
#
# run: 
# 	./bin/app
#
# # Хорошая практика: очистка объектных файлов
# clean:=\
# 	rm -f $(OBJS)
# clear:
# 	rm -f $(OBJS)
#
#
# .PHONY: all run clean
#
#
