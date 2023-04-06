idf_component_register(SRCS "main.c" "joy_stick.c" "web_server.c" "web_client.c"
	INCLUDE_DIRS "."
    EMBED_FILES "../html/error.html"
                "../html/favicon.ico"
                "../html/main.js"
                "../html/root.html"
                "../html/main.css")
