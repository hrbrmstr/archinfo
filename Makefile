archinfo:
	$(CC) archinfo.c -o x86_app -target x86_64-apple-macos10.12
	$(CC) archinfo.c -o arm_app -target arm64-apple-macos11
	lipo -create -output archinfo x86_app arm_app && rm x86_app arm_app

clean:
	rm -f archinfo

test: archinfo
	@./archinfo        | grep -q tccd     && echo "Columns: PASSED" || "Columns: FAILED"
	@./archinfo --json | grep -q 'tccd"}' && echo "   JSON: PASSED" || "   JSON: FAILED"