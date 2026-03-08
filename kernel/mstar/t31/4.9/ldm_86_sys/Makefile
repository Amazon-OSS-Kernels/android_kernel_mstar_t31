#
# Makefile for MStar device drivers.
#

obj-y						+= drv/


clean:
	@find -L \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.ko.*' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' -o -name '*.mod'\
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name modules.builtin -o -name '.tmp_*.o.*' \
		-o -name '*.gcno' -o -name 'Module.symvers' \) -type f -print | xargs rm -f
#	@find -L \( -name '.tmp_versions' \) -print | xargs rmdir
	
ldmkover:
	@echo "LIBCODE:Kernel LDM KO" > lib_version.txt
	@echo "LIBVER:2.$(shell date +%s)" >> lib_version.txt
	@echo "BUILDHASH:$(shell perl get_last_commit.pl $(shell dirname $(shell pwd)))" >> lib_version.txt
	@echo "BUILDDATE:$(shell date +%Y%m%d%H)" >> lib_version.txt
	@find . -name 'mdrv-ldm.ko' -type f | xargs -i $(OBJCOPY) --remove-section .mmodule_version {}
	@find . -name 'mdrv-ldm.ko' -type f | xargs -i $(OBJCOPY) --add-section .mmodule_version=./lib_version.txt {}
	@rm lib_version.txt