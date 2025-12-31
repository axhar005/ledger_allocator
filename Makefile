#--- LIBRARY NAME ---#
NAME = ledger
LIB_NAME = lib$(NAME).a

#--- COMMAND VARIABLES ---#
CC = gcc
CFLAGS = -Wall -Wextra -Werror #-fsanitize=address
RM = rm -f
MK = mkdir -p
AR = ar rcs

#--- COLORS ---#
GREEN	=	\033[1;32m
CYAN	=	\033[1;36m
RED		=	\033[1;31m
RESET 	= 	\033[0m

#--- STRING ---#
CLEANING := "$(GREEN)[+] Cleaning Completed$(RESET)"
SUCCESS := "$(GREEN)[+] $(NAME) Successfully Compiled!$(RESET)"
LIB_SUCCESS := "$(GREEN)[+] $(LIB_NAME) Successfully Compiled!$(RESET)"

#--- SOURCES ---#
SRC_EXEC = main.c
SRC_LIB = ledger.c
SRCDIR = src
INCDIR = include
OBJDIR = object
BINDIR = bin

#--- OBJECTS ---#
OBJ_EXEC = $(addprefix $(OBJDIR)/, $(SRC_EXEC:.c=.o))
OBJ_LIB = $(addprefix $(OBJDIR)/, $(SRC_LIB:.c=.o))

#--- RULES ---#
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -I$(INCDIR) -I. -c $< -o $@
	@echo "$(CYAN)[-] Compiling: $@$(RESET)"

all: $(BINDIR)/$(NAME) $(BINDIR)/$(LIB_NAME)

$(OBJDIR):
	@$(MK) $(OBJDIR)

$(BINDIR):
	@$(MK) $(BINDIR)
	
$(BINDIR)/$(NAME): $(OBJ_EXEC) $(OBJ_LIB) | $(BINDIR)
	@$(CC) $(CFLAGS) -o $(BINDIR)/$(NAME) $(OBJ_EXEC) $(OBJ_LIB)
	@echo $(SUCCESS)

$(BINDIR)/$(LIB_NAME): $(OBJ_LIB) | $(BINDIR)
	@$(AR) $(BINDIR)/$(LIB_NAME) $(OBJ_LIB)
	@echo $(LIB_SUCCESS)

run: $(BINDIR)/$(NAME)
	@./$(BINDIR)/$(NAME)

clean:
	@$(RM) $(OBJ_EXEC) $(OBJ_LIB)
	@$(RM)r $(OBJDIR)
	@echo $(CLEANING)

fclean: clean	
	@$(RM) $(BINDIR)/$(NAME)
	@$(RM) $(BINDIR)/$(LIB_NAME)
	@$(RM)r $(BINDIR)
	@echo $(CLEANING)

re: fclean all

.PHONY: all clean fclean re