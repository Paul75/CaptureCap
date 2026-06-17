# =============================================
# Makefile pour CaptureCap (Android)
# =============================================

# --- Détection des couleurs ---
ifeq ($(shell printf "\033[0;32m" | grep -o "\\[0;32m"),)
    GREEN := \033[0;32m
    YELLOW := \033[1;33m
    RED := \033[0;31m
    BLUE := \033[0;34m
    CYAN := \033[0;36m
    BOLD := \033[1m
    NC := \033[0m
else
    GREEN :=
    YELLOW :=
    RED :=
    BLUE :=
    CYAN :=
    BOLD :=
    NC :=
endif

# --- Variables ---
GRADLE := _JAVA_OPTIONS="-Djava.net.preferIPv6Addresses=true" ./gradlew
ADB := adb

PACKAGE_NAME := com.yepgoryo.CaptureCap
MAIN_ACTIVITY := com.yepgoryo.CaptureCap.MainActivity
APK_DEBUG_PATH := app/build/outputs/apk/debug/app-debug.apk

# --- Cible par défaut ---
default: help

# --- Aide ---
.PHONY: help
help:
	@printf "$(BLUE)=============================================$(NC)\n"
	@printf "$(BOLD)$(BLUE)Makefile pour CaptureCap$(NC)\n"
	@printf "$(BLUE)=============================================$(NC)\n"
	@printf "\n"
	@printf "$(BOLD)Utilisation :$(NC) make [target]\n"
	@printf "\n"
	@printf "$(BOLD)Targets disponibles :$(NC)\n"
	@printf "  $(GREEN)all$(NC)          - Nettoie, compile, installe et lance l'app\n"
	@printf "  $(GREEN)clean$(NC)        - Nettoie le projet\n"
	@printf "  $(GREEN)build$(NC)        - Compile l'APK en mode debug\n"
	@printf "  $(GREEN)install$(NC)      - Installe l'APK debug sur l'appareil connecté\n"
	@printf "  $(GREEN)run$(NC)          - Lance l'application\n"
	@printf "  $(GREEN)uninstall$(NC)    - Désinstalle l'application\n"
	@printf "  $(GREEN)rebuild$(NC)      - Nettoie, compile, installe et lance\n"
	@printf "  $(GREEN)logs$(NC)         - Affiche les logs filtrés pour cette app\n"
	@printf "  $(GREEN)devices$(NC)      - Liste les appareils connectés\n"
	@printf "  $(GREEN)check-adb$(NC)    - Vérifie qu'un appareil est connecté\n"
	@printf "  $(GREEN)help$(NC)         - Affiche cette aide\n"
	@printf "\n"
	@printf "$(BOLD)Exemples :$(NC)\n"
	@printf "  make         # Affiche cette aide\n"
	@printf "  make all     # Nettoie, compile, installe et lance\n"
	@printf "  make logs    # Affiche les logs en temps réel\n"
	@printf "$(BLUE)=============================================$(NC)\n"

# --- Vérification ADB ---
.PHONY: check-adb
check-adb:
	@if ! $(ADB) devices | grep -q "device$$"; then \
		printf "$(RED)Erreur : Aucun appareil Android connecte ou debogage USB non active.$(NC)\n"; \
		printf "   -> Branchez un appareil et activez le $(BOLD)debogage USB$(NC).\n"; \
		exit 1; \
	fi

# --- Nettoyage ---
.PHONY: clean
clean:
	@printf "$(YELLOW)Nettoyage du projet...$(NC)\n"
	$(GRADLE) clean

# --- Compilation ---
.PHONY: build
build: check-adb
	@printf "$(YELLOW)Compilation de l'APK (debug)...$(NC)\n"
	$(GRADLE) assembleDebug

# --- Installation ---
.PHONY: install
install: check-adb
	@printf "$(YELLOW)Installation de l'APK (debug)...$(NC)\n"
	$(ADB) install -r -t $(APK_DEBUG_PATH)

# --- Execution ---
.PHONY: run
run: check-adb
	@printf "$(YELLOW)Lancement de l'application...$(NC)\n"
	$(ADB) shell am start -n $(PACKAGE_NAME)/$(MAIN_ACTIVITY)

# --- Desinstallation ---
.PHONY: uninstall
uninstall: check-adb
	@printf "$(YELLOW)Desinstallation de l'application...$(NC)\n"
	$(ADB) uninstall $(PACKAGE_NAME)

# --- Logs ---
.PHONY: logs
logs: check-adb
	@printf "$(YELLOW)Affichage des logs (Ctrl+C pour arreter)...$(NC)\n"
	$(ADB) logcat | grep -E "$(PACKAGE_NAME)|$(shell echo $(PACKAGE_NAME) | tr '.' '/')"

# --- Appareils connectes ---
.PHONY: devices
devices:
	@printf "$(YELLOW)Appareils connectes :$(NC)\n"
	$(ADB) devices

# --- Cibles combinees ---
.PHONY: all rebuild
all: clean build install run
rebuild: clean build install run
