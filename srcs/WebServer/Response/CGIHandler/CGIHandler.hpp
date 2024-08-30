/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: uahmed <uahmed@student.hive.fi>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/09 10:52:47 by uahmed            #+#    #+#             */
/*   Updated: 2024/08/09 10:54:29 by uahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <ctime>
#include <sys/wait.h>
#include "Request.hpp"

#define PIPES 2
#define WRITEND 1
#define READEND 0
#define CODE404 "404"
#define CODE500 "500"
#define PYTHON3 "/bin/python3"
#define ERROR "\033[31ERROR: \033[0"

class   CGIHandler
{
    public:
        CGIHandler(const Request& request);
        ~CGIHandler() = default;

        std::string      getCGIResponse( void ) const;
    private:
        const Request&   _request;
        std::string      _response;
        std::string      _path;
        int              _output_pipe[PIPES];
        int              _input_pipe[PIPES];

        bool            validateExecutable( void );
        bool            parentWaitForChild(pid_t pid);
        void            executeScript( void );
        void            child( void );
        void            parent( pid_t pid );
        void            childSetEnvp( char const *envp[] );
};
