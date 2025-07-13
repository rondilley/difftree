bin_PROGRAMS = dt
dt_SOURCES = dt.c dt.h processDir.c processDir.h hash.c hash.h parser.c parser.h fileHandlers.c fileHandlers.h noftw.c noftw.h mem.c mem.h util.c util.h md5.c md5.h sha256.c sha256.h ../include/sysdep.h ../include/config.h ../include/common.h Makefile ../Makefile
dt_LDADD = -lz

# Static Analysis Targets
C_SOURCES = dt.c processDir.c hash.c parser.c fileHandlers.c noftw.c mem.c util.c md5.c sha256.c

.PHONY: static-analysis cppcheck clang-tidy scan-build splint

# Run all available static analysis tools
static-analysis:
	@echo "Running static analysis..."
if HAVE_CPPCHECK
	@$(MAKE) cppcheck
endif
if HAVE_CLANG_TIDY  
	@$(MAKE) clang-tidy
endif
if HAVE_SCAN_BUILD
	@$(MAKE) scan-build
endif
if HAVE_SPLINT
	@$(MAKE) splint
endif
	@echo "Static analysis complete."

# cppcheck - comprehensive static analysis
cppcheck:
if HAVE_CPPCHECK
	@echo "Running cppcheck..."
	@$(CPPCHECK) $(CPPCHECK_FLAGS) \
		--xml --xml-version=2 \
		--output-file=cppcheck-report.xml \
		$(C_SOURCES) || true
	@$(CPPCHECK) $(CPPCHECK_FLAGS) $(C_SOURCES) || true
	@echo "cppcheck analysis complete. Report saved to cppcheck-report.xml"
else
	@echo "cppcheck not available"
endif

# clang-tidy - clang-based linter
clang-tidy:
if HAVE_CLANG_TIDY
	@echo "Running clang-tidy..."
	@for src in $(C_SOURCES); do \
		echo "Analyzing $$src..."; \
		$(CLANG_TIDY) $(CLANG_TIDY_FLAGS) $$src -- $(CPPFLAGS) $(CFLAGS) || true; \
	done
	@echo "clang-tidy analysis complete."
else
	@echo "clang-tidy not available"
endif

# scan-build - clang static analyzer
scan-build:
if HAVE_SCAN_BUILD
	@echo "Running scan-build..."
	@$(MAKE) clean
	@$(SCAN_BUILD) $(SCAN_BUILD_FLAGS) -o scan-build-report $(MAKE) all
	@echo "scan-build analysis complete. Report saved to scan-build-report/"
else
	@echo "scan-build not available"
endif

# splint - annotation-assisted static checker
splint:
if HAVE_SPLINT
	@echo "Running splint..."
	@for src in $(C_SOURCES); do \
		echo "Analyzing $$src..."; \
		$(SPLINT) $(SPLINT_FLAGS) $(CPPFLAGS) $$src || true; \
	done
	@echo "splint analysis complete."
else
	@echo "splint not available"
endif

# Clean static analysis outputs
clean-static-analysis:
	@echo "Cleaning static analysis reports..."
	@rm -f cppcheck-report.xml
	@rm -rf scan-build-report/
	@echo "Static analysis reports cleaned."

clean-local: clean-static-analysis
