.PHONY: ----- default help

ifeq "$(origin OS)" "environment"
GODOT = ..\Godot_v4.2.2-stable_win64.exe
else
GODOT = godot
endif

default: help

-----: ## ----- Utility -----

help:  ## This help page
ifeq "$(origin OS)" "environment"
	@powershell -Command '& {Get-Content .\Makefile | Where-Object{$$_ -match "^[a-zA-Z_-]+:.*?##"} | ForEach-Object{$$target, $$desc = ($$_ -split " ## "); $$target = ($$target -split ":")[0]; Write-Host -NoNewline -ForegroundColor Green "$$target"; Write-Host ": $$desc"}}'
else
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST);
endif

clean: ## Clear build artifacts and reinitialize submodules
ifeq "$(origin OS)" "environment"
	@git submodule deinit -f godot-cpp
	@git clean -dfx --exclude='.godot\' --exclude='.vscode\'
	@powershell -Command "& {Remove-Item -ErrorAction Ignore extension_api.json}";
	@git submodule update --init --remote godot-cpp
else
	@git submodule deinit -f godot-cpp
	@git clean -dfx --exclude='.godot/'
	@rm -rf extension_api.json;
	@git submodule update --init --remote godot-cpp
endif

extension_api_json: ## Dump extension_api.json for current version of Godot
ifeq "$(origin OS)" "environment"
	@powershell -Command "& {Write-Host -NoNewLine 'PowerShell: Dumping Extension API for godot version '; $(GODOT) --version;}";
	@powershell -Command "& {cd godot-cpp; $(GODOT) --dump-extension-api-with-docs | Out-Null; Move-Item -Force 'extension_api.json' '..\'; Write-Host 'Generated extension_api.json'}";
else
	@echo -n "Bash: Dumping Extension API for godot version ";
	@cd godot-cpp && $(GODOT) --version;
	@cd godot-cpp && $(GODOT) --dump-extension-api-with-docs && mv extension_api.json ../ && echo "Generated extension_api.json";
endif

-----: ## ----- Godot-CPP builds -----

godot_cpp_linux: extension_api_json ## Build Godot CPP in Linux
	@cd godot-cpp && scons platform=linux arch=x86_32 custom_api_file=../extension_api.json
	@cd godot-cpp && scons platform=linux arch=x86_64 custom_api_file=../extension_api.json

godot_cpp_linux_debug: extension_api_json ## Build Godot CPP in Linux with extra debugging
	@cd godot-cpp && scons platform=linux arch=x86_32 custom_api_file=../extension_api.json debug_symbols=true
	@cd godot-cpp && scons platform=linux arch=x86_64 custom_api_file=../extension_api.json debug_symbols=true

godot_cpp_mac: extension_api_json ## Build Godot CPP in MacOS
	@cd godot-cpp && scons platform=macos arch=universal custom_api_file=../extension_api.json

godot_cpp_mac_debug: extension_api_json ## Build Godot CPP in MacOS with extra debugging
	@cd godot-cpp && scons platform=macos arch=universal custom_api_file=../extension_api.json debug_symbols=true

godot_cpp_windows: extension_api_json ## Build Godot CPP in Windows
	@cd godot-cpp && scons platform=windows arch=x86_32 custom_api_file=../extension_api.json use_mingw=true
	@cd godot-cpp && scons platform=windows arch=x86_64 custom_api_file=../extension_api.json use_mingw=true

godot_cpp_windows_debug: extension_api_json ## Build Godot CPP in Windows with extra debugging
	@cd godot-cpp && scons platform=windows arch=x86_32 custom_api_file=../extension_api.json use_mingw=true debug_symbols=true
	@cd godot-cpp && scons platform=windows arch=x86_64 custom_api_file=../extension_api.json use_mingw=true debug_symbols=true

-----: ## ----- GDExtension builds -----

gdextension_linux_debug: ## Build GDExtensions (Debug) for Linux.
	@#scons platform=linux target=template_debug arch=x86_32 debug_symbols=true
	scons platform=linux target=template_debug arch=x86_64 debug_symbols=true
	@echo "\nBuild the 32-bit extension manually to avoid tripping the stack smashing bug.\n\tscons platform=linux target=template_debug arch=x86_32 debug_symbols=true"

gdextension_linux_release: ## Build GDExtensions (Release) for Linux.
	scons platform=linux target=template_release arch=x86_32
	scons platform=linux target=template_release arch=x86_64

gdextension_windows_debug: ## Build GDExtensions (Debug) for Windows.
	scons platform=windows target=template_debug arch=x86_64 debug_symbols=true
	@powershell -Command "& {Write-Output \"Build the 32-bit extension manually to avoid tripping the stack smashing bug. scons platform=windows target=template_debug arch=x86_32 debug_symbols=true\"}";

gdextension_windows_release: ## Build GDExtensions (release) for Windows.
	scons platform=windows target=template_release arch=x86_32
	scons platform=windows target=template_release arch=x86_64

-----: ## ----- Tests -----

tests: ## Run doctest unit tests for the GDExtension
ifeq "$(origin OS)" "environment"
	cmake.exe -S test -B .cmake -G"MinGW Makefiles"
	cmake.exe --build .cmake --verbose
else
	cmake -S ./test -B ./.cmake
	cmake --build ./.cmake --verbose
endif