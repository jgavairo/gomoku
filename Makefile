# ================================ TARGETS =================================== #
.PHONY: all build clean fclean re test debug release help check-deps
.PHONY: install uninstall lib evaluate test-ai-improvements test-ai-tactical test-candidate-visual benchmark-candidates SFML check-deps-auto
.DEFAULT_GOAL := all

# =============================== COMPILER ================================== #
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -O2 -Wpedantic -MMD -MP
CXXFLAGS += -Wunused -Wunused-function -Wunused-variable -Wunused-parameter
CXXFLAGS += -Wunreachable-code -Wshadow -Wconversion -Wmissing-declarations

DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG

# Enable parallel compilation by default
MAKEFLAGS += -j$(shell nproc)

# Verbose mode (show full commands)
ifdef VERBOSE
	Q =
else
	Q = @
endif

# ========================== SYSTEM DETECTION ============================== #
UNAME_S := $(shell uname -s)

# ============================= SFML CONFIG ================================ #
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew
    SFML_DIR = /opt/homebrew
    SFML_INCLUDE = -I$(SFML_DIR)/include
    SFML_LIBS = -L$(SFML_DIR)/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
    SFML_RPATH = -Wl,-rpath,$(SFML_DIR)/lib
else
    # Linux: use a configurable prefix
    SFML_DIR ?= $(shell if [ -d "$(HOME)/local" ]; then echo "$(HOME)/local"; else echo "/usr"; fi)
    SFML_INCLUDE = -I$(SFML_DIR)/include
    SFML_LIBS = -L$(SFML_DIR)/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
    SFML_RPATH = -Wl,-rpath,$(SFML_DIR)/lib
endif

# ============================= DIRECTORIES ================================ #
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Target names
TARGET = bin/Gomoku                      # GUI executable (SFML)
LIB_NAME = lib/libgomoku_logic.a         # static library logic/AI
TEST_BIN = bin/tests_runner              # unit test binary (without SFML)
EVAL_BIN = bin/evaluate_runner           # AI evaluation binary (without SFML)
AI_IMPROVEMENTS_BIN = bin/ai_improvements_runner  # AI improvements test binary
AI_TACTICAL_BIN = bin/ai_tactical_runner # AI tactical tests binary (captures)
CANDIDATE_VISUAL_BIN = bin/candidate_visual_runner # CandidateGenerator visual tests
BENCHMARK_CANDIDATES_BIN = bin/benchmark_candidates # CandidateGenerator benchmark

# ================================ SOURCES =================================== #
CORE_SRC = \
	$(SRC_DIR)/gomoku/core/Board.cpp \
	$(SRC_DIR)/gomoku/core/CaptureEngine.cpp \
	$(SRC_DIR)/gomoku/core/PatternAnalyzer.cpp \
	$(SRC_DIR)/gomoku/core/Zobrist.cpp \
	$(SRC_DIR)/gomoku/core/BoardState.cpp \
	$(SRC_DIR)/gomoku/core/Types.cpp \
	$(SRC_DIR)/gomoku/ai/MinimaxSearch.cpp \
	$(SRC_DIR)/gomoku/ai/MinimaxSearchEngine.cpp \
	$(SRC_DIR)/gomoku/ai/CandidateGenerator.cpp \
	$(SRC_DIR)/gomoku/ai/Evaluator.cpp \
	$(SRC_DIR)/gomoku/ai/SearchHelpers.cpp \
	$(SRC_DIR)/gomoku/ai/MoveOrderer.cpp \
	$(SRC_DIR)/gomoku/application/SessionController.cpp \
	$(SRC_DIR)/gomoku/application/GameService.cpp \
	$(SRC_DIR)/gomoku/application/MoveValidator.cpp \
	$(SRC_DIR)/util/Logger.cpp

GUI_SRC = \
	main.cpp \
	$(SRC_DIR)/ui/Button.cpp \
	$(SRC_DIR)/scene/AScene.cpp \
	$(SRC_DIR)/scene/GameScene.cpp \
	$(SRC_DIR)/scene/GameSelect.cpp \
	$(SRC_DIR)/scene/Settings.cpp \
	$(SRC_DIR)/scene/MainMenu.cpp \
	$(SRC_DIR)/gui/GameWindow.cpp \
	$(SRC_DIR)/gui/GameBoardRenderer.cpp \
	$(SRC_DIR)/gui/ResourceManager.cpp \
	$(SRC_DIR)/util/Preferences.cpp

UNIT_TEST_SRC = \
	tests/test_runner.cpp \
	tests/unit/test_board_basics.cpp \
	tests/unit/test_alignment.cpp \
	tests/unit/test_captures.cpp \
	tests/unit/test_double_three.cpp \
	tests/unit/test_legality.cpp \
	tests/unit/test_reversibility.cpp

EVAL_TEST_SRC = \
	tests/evaluate_runner.cpp \
	tests/evaluation/ai_performance.cpp

AI_IMPROVEMENTS_TEST_SRC = \
	tests/ai_improvements_runner.cpp \
	tests/evaluation/ai_improvements.cpp

AI_TACTICAL_TEST_SRC = \
	tests/evaluation/ai_tactical_runner.cpp \
	tests/evaluation/ai_tactical_tests.cpp

CANDIDATE_VISUAL_TEST_SRC = \
	tests/evaluation/candidate_visual_runner.cpp \
	tests/evaluation/candidate_visual_tests.cpp

BENCHMARK_CANDIDATES_SRC = \
	tests/benchmark_candidates.cpp

# ================================ OBJECTS =================================== #
CORE_OBJ = $(CORE_SRC:%.cpp=$(OBJ_DIR)/%.o)
GUI_OBJ  = $(GUI_SRC:%.cpp=$(OBJ_DIR)/%.o)
UNIT_TEST_OBJ = $(UNIT_TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
EVAL_TEST_OBJ = $(EVAL_TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
AI_IMPROVEMENTS_TEST_OBJ = $(AI_IMPROVEMENTS_TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
AI_TACTICAL_TEST_OBJ = $(AI_TACTICAL_TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
CANDIDATE_VISUAL_TEST_OBJ = $(CANDIDATE_VISUAL_TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
BENCHMARK_CANDIDATES_OBJ = $(BENCHMARK_CANDIDATES_SRC:%.cpp=$(OBJ_DIR)/%.o)

# Generate dependency files (.d)
DEPFILES := $(CORE_OBJ:%.o=%.d) $(GUI_OBJ:%.o=%.d) $(UNIT_TEST_OBJ:%.o=%.d) $(EVAL_TEST_OBJ:%.o=%.d) $(AI_IMPROVEMENTS_TEST_OBJ:%.o=%.d)

# ================================= COLORS =================================== #
RESET = \033[0m
BOLD = \033[1m
RED = \033[31m
GREEN = \033[32m
YELLOW = \033[33m
BLUE = \033[34m
MAGENTA = \033[35m
CYAN = \033[36m

# =============================== MESSAGES ================================== #
MSG_COMPILE = $(CYAN)[COMPILE]$(RESET)
MSG_LINK = $(GREEN)[LINK]$(RESET)
MSG_CLEAN = $(YELLOW)[CLEAN]$(RESET)
MSG_INFO = $(BLUE)[INFO]$(RESET)
MSG_SUCCESS = $(GREEN)[SUCCESS]$(RESET)
MSG_ERROR = $(RED)[ERROR]$(RESET)
MSG_AR = $(MAGENTA)[AR]$(RESET)
MSG_INSTALL = $(GREEN)[INSTALL]$(RESET)
MSG_UNINSTALL = $(YELLOW)[UNINSTALL]$(RESET)

# ============================= MAIN TARGETS =============================== #
all: check-deps-auto $(TARGET) install

build: $(TARGET)

# ======================== DEPENDENCY CHECK ================================ #
check-deps-auto:
	@printf "$(MSG_INFO) Checking SFML dependencies...\n"
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		if ! brew list sfml >/dev/null 2>&1; then \
			printf "$(MSG_INFO) SFML not found, installing via Homebrew...\n"; \
			$(MAKE) SFML; \
		else \
			printf "$(MSG_SUCCESS) SFML found via Homebrew\n"; \
		fi; \
	else \
		if [ ! -f "$(SFML_DIR)/lib/libsfml-graphics.so" ] && [ ! -f "$(SFML_DIR)/lib64/libsfml-graphics.so" ]; then \
			printf "$(MSG_INFO) SFML not found in $(SFML_DIR), attempting installation...\n"; \
			$(MAKE) SFML; \
		else \
			printf "$(MSG_SUCCESS) SFML found in $(SFML_DIR)\n"; \
		fi; \
	fi

check-deps:
	@printf "$(MSG_INFO) System: $(BOLD)$(UNAME_S)$(RESET)\n"
	@printf "$(MSG_INFO) SFML: $(BOLD)$(SFML_DIR)$(RESET)\n"
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		if brew list sfml >/dev/null 2>&1; then \
			printf "$(MSG_SUCCESS) SFML installed via Homebrew\n"; \
			ls $(SFML_DIR)/lib/libsfml* 2>/dev/null | head -3; \
		else \
			printf "$(MSG_ERROR) SFML not installed via Homebrew\n"; \
			printf "$(MSG_INFO) Execute: $(BOLD)brew install sfml$(RESET)\n"; \
		fi; \
	else \
		if [ -f "$(SFML_DIR)/lib/libsfml-graphics.so" ] || [ -f "$(SFML_DIR)/lib64/libsfml-graphics.so" ]; then \
			printf "$(MSG_SUCCESS) SFML found in $(SFML_DIR)\n"; \
			ls $(SFML_DIR)/lib/libsfml* $(SFML_DIR)/lib64/libsfml* 2>/dev/null | head -3; \
		else \
			printf "$(MSG_ERROR) SFML not found in $(SFML_DIR)\n"; \
			printf "$(MSG_INFO) Install SFML (ex: $(BOLD)sudo apt install libsfml-dev$(RESET)) or adjust SFML_DIR\n"; \
		fi; \
	fi

# ============================== BUILD RULES =============================== #
$(LIB_NAME): $(CORE_OBJ)
	@mkdir -p $(dir $@)
	@printf "$(MSG_AR) Building static library $(BOLD)$@$(RESET)...\n"
	$(Q)ar rcs $@ $(CORE_OBJ)
	@printf "$(MSG_SUCCESS) Library $(BOLD)$@$(RESET) created successfully!\n"

$(TARGET): $(GUI_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(GUI_OBJ) $(LIB_NAME) $(SFML_LIBS) $(SFML_RPATH) $(LDFLAGS) -o $@
	@printf "$(MSG_SUCCESS) $(BOLD)$@$(RESET) compiled successfully!\n"

$(TEST_BIN): $(UNIT_TEST_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking tests $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(UNIT_TEST_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) Tests $(BOLD)$@$(RESET) compiled successfully!\n"

$(EVAL_BIN): $(EVAL_TEST_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking evaluator $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(EVAL_TEST_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) Evaluator $(BOLD)$@$(RESET) compiled successfully!\n"

$(AI_IMPROVEMENTS_BIN): $(AI_IMPROVEMENTS_TEST_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking AI improvements tests $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(AI_IMPROVEMENTS_TEST_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) AI improvements tests $(BOLD)$@$(RESET) compiled successfully!\n"

$(AI_TACTICAL_BIN): $(AI_TACTICAL_TEST_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking AI tactical tests $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(AI_TACTICAL_TEST_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) AI tactical tests $(BOLD)$@$(RESET) compiled successfully!\n"

$(CANDIDATE_VISUAL_BIN): $(CANDIDATE_VISUAL_TEST_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking CandidateGenerator visual tests $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(CANDIDATE_VISUAL_TEST_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) CandidateGenerator visual tests $(BOLD)$@$(RESET) compiled successfully!\n"

$(BENCHMARK_CANDIDATES_BIN): $(BENCHMARK_CANDIDATES_OBJ) $(LIB_NAME)
	@mkdir -p $(dir $@)
	@printf "$(MSG_LINK) Linking CandidateGenerator benchmark $(BOLD)$@$(RESET)...\n"
	$(Q)$(CXX) $(BENCHMARK_CANDIDATES_OBJ) $(LIB_NAME) -o $@
	@printf "$(MSG_SUCCESS) CandidateGenerator benchmark $(BOLD)$@$(RESET) compiled successfully!\n"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@printf "$(MSG_COMPILE) $<\n"
	$(Q)$(CXX) $(CXXFLAGS) -Isrc -I$(INCLUDE_DIR) $(SFML_INCLUDE) -c $< -o $@

# ============================ BUILD VARIANTS ============================== #
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@printf "$(MSG_SUCCESS) Debug build completed!\n"

release: CXXFLAGS += $(RELEASE_FLAGS)
release: fclean $(TARGET)
	@printf "$(MSG_SUCCESS) Release build completed!\n"

# ============================= LIBRARY TARGET ============================= #
lib: $(LIB_NAME)

# ============================== UTILITIES ================================== #
test: $(TEST_BIN)
	@printf "\n$(MSG_INFO) Running unit tests...\n\n"
	@./$(TEST_BIN)

evaluate: $(EVAL_BIN)
	@printf "\n$(MSG_INFO) Running AI evaluation and performance analysis...\n"
	@printf "$(MSG_INFO) Tip: Use '$(BOLD)make evaluate ARGS=\"-d\"$(RESET)' for detailed debug logs\n\n"
	@./$(EVAL_BIN) $(ARGS)

test-ai-improvements: $(AI_IMPROVEMENTS_BIN)
	@printf "\n$(MSG_INFO) Running AI improvements tests...\n\n"
	@./$(AI_IMPROVEMENTS_BIN)

test-ai-tactical: $(AI_TACTICAL_BIN)
	@printf "\n$(MSG_INFO) Running AI tactical tests (captures @ 500ms)...\n\n"
	@./$(AI_TACTICAL_BIN)

test-candidate-visual: $(CANDIDATE_VISUAL_BIN)
	@printf "\n$(MSG_INFO) Running CandidateGenerator visual tests...\n\n"
	@./$(CANDIDATE_VISUAL_BIN)

benchmark-candidates: $(BENCHMARK_CANDIDATES_BIN)
	@printf "\n$(MSG_INFO) Running CandidateGenerator benchmark...\n\n"
	@./$(BENCHMARK_CANDIDATES_BIN)

# ========================== INSTALL/UNINSTALL ============================= #
install: $(TARGET)
	@printf "$(MSG_INSTALL) Installing Gomoku\n"
	@printf "$(MSG_INSTALL) Binary: $< -> ~/bin/\n"
	@mkdir -p ~/bin
	@cp $(TARGET) ~/bin/
	@cd desktop && ./install_desktop.sh
	@printf "$(MSG_SUCCESS) Installation completed!\n"

uninstall:
	@cd desktop && ./uninstall_desktop.sh
	@rm -f ~/bin/$(notdir $(TARGET))

# =============================== CLEANING ================================== #
clean:
	@deleted=0; \
	if [ -d "$(BUILD_DIR)" ]; then \
		printf "$(MSG_CLEAN) Removing build directory $(BUILD_DIR)...\n"; \
		rm -rf "$(BUILD_DIR)"; \
		deleted=1; \
	fi; \
	if [ -d "logs/" ]; then \
		printf "$(MSG_CLEAN) Removing logs directory logs/...\n"; \
		rm -rf "logs/"; \
		deleted=1; \
	fi; \
	if [ $$deleted -eq 1 ]; then \
		printf "$(MSG_SUCCESS) Clean completed: removed files/directories.\n"; \
	else \
		printf "$(MSG_INFO) Clean completed: nothing to remove.\n"; \
	fi

fclean: uninstall clean
	@deleted=0; \
	if [ -d "bin/" ]; then \
		printf "$(MSG_CLEAN) Removing binaries directory bin/...\n"; \
		rm -rf "bin/"; \
		deleted=1; \
	fi; \
	if [ -d "lib/" ]; then \
		printf "$(MSG_CLEAN) Removing libraries directory lib/...\n"; \
		rm -rf "lib/"; \
		deleted=1; \
	fi; \
	if [ -d "$(HOME)/.config/gomoku" ]; then \
		rm -rf "$(HOME)/.config/gomoku"; \
		printf "$(MSG_CLEAN) Removed user config: $(HOME)/.config/gomoku\n"; \
		deleted=1; \
	fi; \
	if [ $$deleted -eq 1 ]; then \
		printf "$(MSG_SUCCESS) Full clean completed: removed files/directories.\n"; \
	else \
		printf "$(MSG_INFO) Full clean completed: nothing to remove.\n"; \
	fi

re: fclean
	@$(MAKE) all

# ============================ SFML SETUP ================================== #
SFML:
	@printf "$(MSG_INFO) Setting up SFML...\n"
	@./scripts/setup_sfml.sh

# ================================= HELP ==================================== #
help:
	@printf "$(BOLD)Gomoku Project Makefile$(RESET)\n"
	@printf "\n$(BOLD)Build Targets:$(RESET)\n"
	@printf "  $(GREEN)all$(RESET)       - Check deps, build and install automatically (default)\n"
	@printf "  $(GREEN)build$(RESET)     - Build main GUI executable only (no auto-install)\n"
	@printf "  $(GREEN)lib$(RESET)       - Build core library only\n"
	@printf "  $(GREEN)debug$(RESET)     - Build with debug symbols\n"
	@printf "  $(GREEN)release$(RESET)   - Build optimized release version\n"
	@printf "  $(GREEN)test$(RESET)      - Build and run unit tests\n"
	@printf "  $(GREEN)evaluate$(RESET)  - Build and run AI evaluation\n"
	@printf "\n$(BOLD)Clean Targets:$(RESET)\n"
	@printf "  $(GREEN)clean$(RESET)     - Remove build directory and logs\n"
	@printf "  $(GREEN)fclean$(RESET)    - Remove all generated files\n"
	@printf "  $(GREEN)re$(RESET)        - Rebuild everything (fclean + all)\n"
	@printf "\n$(BOLD)Setup Targets:$(RESET)\n"
	@printf "  $(GREEN)SFML$(RESET)      - Check/install SFML dependencies\n"
	@printf "  $(GREEN)check-deps$(RESET)- Check system dependencies\n"
	@printf "\n$(BOLD)Install Targets:$(RESET)\n"
	@printf "  $(GREEN)install$(RESET)   - Install executable + desktop integration\n"
	@printf "  $(GREEN)uninstall$(RESET) - Remove executable + desktop integration\n"
	@printf "\n$(BOLD)Options:$(RESET)\n"
	@printf "  $(CYAN)VERBOSE=1$(RESET) - Show full compiler commands\n"
	@printf "  $(CYAN)ARGS=\"...\"$(RESET) - Pass arguments to evaluate target\n"
	@printf "\n$(BOLD)System Info:$(RESET)\n"
	@printf "  System:         $(BOLD)$(UNAME_S)$(RESET)\n"
	@printf "  Compiler:       $(BOLD)$(CXX)$(RESET)\n"
	@printf "  SFML:           $(BOLD)$(SFML_DIR)$(RESET)\n"
	@printf "  Parallel jobs:  $(BOLD)$(shell nproc)$(RESET) (automatic)\n"
	@printf "\n$(BOLD)Examples:$(RESET)\n"
	@printf "  make test\n"
	@printf "  make evaluate ARGS=\"-d\"\n"
	@printf "  make debug\n"
	@printf "  make VERBOSE=1\n"

# ========================== DEPENDENCY INCLUSION =========================== #
-include $(wildcard $(DEPFILES))
