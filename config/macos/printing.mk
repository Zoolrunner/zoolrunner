# Cocoa targets from 10.6 onward use the native print panel: the SDK no
# longer exposes the Carbon PDE interfaces. Keep the older target path.
ifeq ($(MOZ_WIDGET_TOOLKIT),cocoa)
ifeq (,$(filter 10.0 10.0.% 10.1 10.1.% 10.2 10.2.% 10.3 10.3.% 10.4 10.4.% 10.5 10.5.%,$(MACOSX_DEPLOYMENT_TARGET)))
MOZ_COCOA_NATIVE_PRINTING = 1
endif
endif
