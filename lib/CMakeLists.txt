set(AUTHLIB "CGI_auth")
aux_source_directory(. LIB_SRC)
add_library(${AUTHLIB} SHARED ${LIB_SRC})
