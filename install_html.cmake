# Copy the files to the destination directory
execute_process(COMMAND ./scripts/cp_html \"${SERVER_NAME}\" \"${HTTPS_DIR}\")
