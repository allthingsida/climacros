PROC=climacros

include ../plugin.mak

# MAKEDEP dependency list ------------------
$(F)climacros$(O) : $(I)loader.hpp $(I)ida.hpp $(I)expr.hpp         \
                    $(I)kernwin.hpp $(I)diskio.hpp $(I)registry.hpp \
                    climacros.cpp
