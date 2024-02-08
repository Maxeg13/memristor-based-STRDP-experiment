# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c wifi.c udp.c i2c_user.c lcd2004.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
