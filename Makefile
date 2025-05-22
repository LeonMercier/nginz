# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: sniemela <sniemela@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/05/21 12:37:52 by sniemela          #+#    #+#              #
#    Updated: 2025/05/22 09:48:24 by sniemela         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

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

NAME = webserv
CPPFLAGS = -Wall -Werror -Wextra -std=c++11
CPP = c++
SOURCES = src/main.cpp 

SRCDIR = src
OBJDIR = obj
OBJS = $(SOURCES:%.c=$(OBJDIR)/%.o)

all: $(NAME)


$(NAME): $(OBJS) 
	@$(CUB3D)
	@$(CPP) $(CFLAGS) $(OBJS) -o $(NAME)
	@$(X_READY)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	@$(CPP) $(CFLAGS) -c $< -o $@
	@$(OBJ_READY)


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