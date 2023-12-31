################################################################################
#
# python-pbr
#
################################################################################

PYTHON_PBR_VERSION = 5.4.3
PYTHON_PBR_SOURCE = pbr-$(PYTHON_PBR_VERSION).tar.gz
PYTHON_PBR_SITE = https://files.pythonhosted.org/packages/99/f1/7807d3409c79905a907f1c616d910c921b2a8e73c17b2969930318f44777
PYTHON_PBR_SETUP_TYPE = setuptools
PYTHON_PBR_LICENSE = Apache-2.0 (module), BSD-3-Clause (test package)
PYTHON_PBR_LICENSE_FILES = LICENSE pbr/tests/testpackage/LICENSE.txt

define PYTHON_PBR_INSTALL_TARGET_FIXUP
	rm -rf $(TARGET_DIR)/usr/lib/python*/site-packages/pbr/tests/testpackage/src/testext.c
endef

PYTHON_PBR_POST_INSTALL_TARGET_HOOKS += PYTHON_PBR_INSTALL_TARGET_FIXUP

$(eval $(python-package))
$(eval $(host-python-package))
