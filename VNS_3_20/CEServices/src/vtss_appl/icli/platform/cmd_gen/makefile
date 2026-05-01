ICLI_CMD_GEN_INCLUDES := -I$(ICLI_ENGINE_DIR)/base \
                         -I$(ICLI_ENGINE_DIR)/platform/cmd_gen \
                         -I$(ICLI_ENGINE_DIR)/platform/porting

ICLI_CMD_GEN_FILES    := $(ICLI_ENGINE_DIR)/base/icli_cmd_gen.c \
                         $(ICLI_ENGINE_DIR)/base/icli_util.c \
                         $(ICLI_ENGINE_DIR)/base/icli_variable.c \
                         $(ICLI_ENGINE_DIR)/platform/cmd_gen/icli_porting_api.c

ICLI_CMD_GEN_DEFINES  := -D___LINUX___

icli_cmd_gen: $(ICLI_CMD_GEN_FILES)
	gcc $(ICLI_CMD_GEN_FILES) $(ICLI_CMD_GEN_INCLUDES) $(ICLI_CMD_GEN_DEFINES) -o $(ICLI_SCRIPT_GEN_DIR)/icli_cmd_gen

clean:
	-rm -f $(ICLI_SCRIPT_GEN_DIR)/icli_cmd_gen
