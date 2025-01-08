idf_component_register(SRCS "main.c" "joy_stick.c" "nvs.c" "web_server.c" "web_client.c"
	INCLUDE_DIRS "."
    EMBED_FILES "../html/error.html"
                "../html/favicon.ico"
                "../html/mario0.png"
                "../html/mario1.png"
                "../html/main.js"
                "../html/root.html"
                "../html/main.css")
