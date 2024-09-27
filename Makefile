SRC = $(shell find srcs -name *.cpp)
OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)
CXX = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic $(addprefix -I, $(shell find srcs -type d)) -MMD -MP
NAME = webserv

DOCKER_COMPOSE_FILE := ./docker-services/docker-compose.yml

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

-include $(DEPS)

clean: down
	$(RM) $(OBJS) $(DEPS)
	find srcs -type f \( -name "*.o" -o -name "*.d" \) -delete

fclean: clean
	$(RM) $(NAME)

re: fclean $(NAME)

eval: up all
	./webserv ./tests/eval.conf

up:
	mkdir -p ./docker-services/homer
	chmod 777 ./docker-services/homer
	docker compose -f $(DOCKER_COMPOSE_FILE) up -d || @docker-compose -f $(DOCKER_COMPOSE_FILE) up -d
	docker compose -f $(DOCKER_COMPOSE_FILE) logs || @docker-compose -f $(DOCKER_COMPOSE_FILE) logs

down:
	@docker compose -f $(DOCKER_COMPOSE_FILE) down

.PHONY: all clean fclean re up down eval
