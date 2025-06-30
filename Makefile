RESET 			= \033[0;39m
ORANGE 			= \e[1m\e[38;5;202m
CYAN_BOLD 		= \033[1;96m
GREEN			= \033[1;92m
GREEN_UNDER		= \033[4;32m
YELLOW 			= \033[0;93m

OBJ_READY		= echo "📥 $(ORANGE)Compiled .o files!$(RESET)"
X_READY			= echo "🤖 $(GREEN)webserv ready!$(RESET)"
CLEANING		= echo "💧 $(CYAN_BOLD)Cleaning...$(RESET)"
CLEANED			= echo "💧 $(CYAN_BOLD)Successfully cleaned object files!$(RESET)"
FCLEANING		= echo "🧼 $(CYAN_BOLD)Deep cleaning...$(RESET)"
FCLEANED		= echo "🧼 $(CYAN_BOLD)Successfully cleaned all executable files!$(RESET)"
REMAKE			= echo "💡 $(GREEN)Successfully rebuilt everything!$(RESET)"
WEBSERV			= echo "🔗 $(YELLOW)Linking webserv...$(RESET)"

COUNT_FILE := .compile_count.tmp

NAME = webserv
CPPFLAGS = -std=c++17 -Wall -Werror -Wextra -g
CPP = c++
SOURCES = src/main.cpp \
			src/autoindex.cpp \
			src/CgiHandler.cpp \
			src/Client.cpp \
			src/ConfigParser.cpp \
			src/event_loop.cpp \
			src/parse_header.cpp \
			src/Request.cpp \
			src/utils.cpp
			
TOTAL := $(words $(SOURCES))
HEADERS = inc/Client.hpp inc/event_loop.hpp \
		  inc/Request.hpp inc/CgiHandler.hpp \
		  inc/utils.hpp

SRCDIR = src
OBJDIR = obj
OBJS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(NAME)


$(NAME): $(OBJS) $(HEADERS)
	@$(OBJ_READY)
	@$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)
	@$(X_READY)
	@rm -f $(COUNT_FILE)


$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) | $(OBJDIR)
	@mkdir -p $(dir $@)
	@$(CPP) $(CPPFLAGS) -c $< -o $@
	@count=$$(cat $(COUNT_FILE) 2>/dev/null || echo 0); \
	count=$$((count + 1)); \
	echo $$count > $(COUNT_FILE); \
	echo "📥 $(ORANGE)Compiled $$count/$(TOTAL) .o files!$(RESET)"


clean:
	@$(CLEANING)
	@rm -rf $(OBJDIR)
	@$(CLEANED)

fclean: clean
	@$(FCLEANING)
	@rm -f $(NAME)
	@$(FCLEANED)

re: fclean all
	@$(REMAKE)

.PHONY: all clean fclean re
