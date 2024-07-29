SRC =	$(shell find srcs -name *.cpp)
OBJS = $(SRC:.cpp=.o)
CXX = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++11 -pedantic -I./headers
NAME = webserv

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean $(NAME)


# TESTS ----

DOCKER_COMPOSE_FILE := ./docker-services/docker-compose.yml

proxy-pass-test: up $(NAME)
	./webserv ./tests/nginx_test.conf

up:
	mkdir -p ./docker-services/homer
	chmod 777 ./docker-services/homer
	docker compose -f $(DOCKER_COMPOSE_FILE) up -d || @docker-compose -f $(DOCKER_COMPOSE_FILE) up -d
	docker compose -f $(DOCKER_COMPOSE_FILE) logs || @docker-compose -f $(DOCKER_COMPOSE_FILE) logs

down:
	@docker compose -f $(DOCKER_COMPOSE_FILE) down || @docker-compose -f $(DOCKER_COMPOSE_FILE) down

# ---


.PHONY: all clean fclean re proxy-pass-test compose-up compose-down
