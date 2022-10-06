# replace this with yours if you want to codesign your own binary
IDENTITY="CBY22P58G8"

archinfo:
	$(CC) archinfo.c -o x86_app -target x86_64-apple-macos10.12
	$(CC) archinfo.c -o arm_app -target arm64-apple-macos11
	lipo -create -output archinfo x86_app arm_app && rm x86_app arm_app

leaks: archinfo
	leaks --readonlyContent -atExit -- ./archinfo | grep LEAK: || true
	leaks --readonlyContent -atExit -- ./archinfo --json | grep LEAK: || true
	leaks --readonlyContent -atExit -- ./archinfo --json | grep LEAK: || true

sign: archinfo
	codesign --force --verify --verbose --sign ${IDENTITY} archinfo

clean:
	rm -f archinfo

install: archinfo
	codesign --force --verify --verbose --sign ${IDENTITY} archinfo
	cp archinfo /usr/local/bin


test: archinfo
	@./archinfo           | grep -q tccd     && echo "Columns: PASSED (list)"           || echo "Columns: FAILED (list)"
	@./archinfo --columns | grep -q tccd     && echo "Columns: PASSED (list, explicit)" || echo "Columns: FAILED (list, explicit)"
	@./archinfo --json    | grep -q 'tccd"}' && echo "   JSON: PASSED (list)"           || echo "   JSON: FAILED (list)"
	@(./archinfo           --pid `pgrep keyboardservicesd` | grep -q '64') && echo "Columns: PASSED (single)"           || echo "Columns: FAILED (single)"
	@(./archinfo --columns --pid `pgrep keyboardservicesd` | grep -q '64') && echo "Columns: PASSED (single, explicit)" || echo "Columns: FAILED (single, explicit)"
	@(./archinfo --json    --pid `pgrep keyboardservicesd` | grep -q '"}') && echo "   JSON: PASSED (single)"           || echo "   JSON: FAILED (single)"
