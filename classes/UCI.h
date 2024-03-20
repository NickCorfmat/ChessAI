#pragma once

/**
 * @class ChessEngineUCI
 * @brief Represents a UCI-compliant chess engine.
 *
 * This class provides an interface for a UCI-compliant chess engine
 * to communicate with a chess GUI or other UCI-compatible programs.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include "Chess.h"

const std::string UCI_CMD_ISREADY = "isready";
const std::string UCI_CMD_UCINEWGAME = "ucinewgame";
const std::string UCI_CMD_POSITION = "position";
const std::string UCI_CMD_GO = "go";
const std::string UCI_CMD_STOP = "stop";
const std::string UCI_CMD_QUIT = "quit";
const std::string UCI_CMD = "uci";
const std::string UCI_CMD_MOVE = "move";
const std::string UCI_CMD_MOVES = "moves";

class UCIInterface
{
public:
    UCIInterface()
    {
        // Initialize any necessary variables or resources here.
    }

    // Function to start the UCI communication loop, this needs to be on a thread.
    void Run(Chess *instance)
    {
        _instance = instance;
        // Send the engine's identity and available options in UCI format.
        SendUCIInfo();
        UCILog("RUNNING NEW GAME");
        _state = "";
        ReadInput();
    }

    // Function to read input from the console.
    void ReadInput()
    {
        std::string input;
        while (true)
        {
            std::getline(std::cin, input);
            // Process the received UCI command.
            ProcessUCICommand(input);
        }
    }

    void SendMove(std::string move)
    {
        std::cout << "bestmove " << move << std::endl;
        std::cout.flush();
        UCILog("bestmove " + std::string(move));
    }

    void UCILog(std::string message);
    void FENtoBoard(const std::string &fen);

private:
    void ProcessMoves(std::istringstream &iss)
    {
        // look for moves
        std::string move;
        iss >> move;
        if (move == UCI_CMD_MOVES)
        {
            while (iss >> move)
            {
                UCILog("move : " + move);
                _instance->UCIMove(move);
            }
            if (!_instance->getCurrentPlayer()->isAIPlayer())
            {
                _instance->endTurn();
            }
        }
    }
    // Function to parse and handle incoming UCI commands.
    void ProcessUCICommand(const std::string &command)
    {
        std::istringstream iss(command);
        std::string token;
        iss >> token;

        UCILog(token);

        if (token == UCI_CMD)
        {
            // Handle the "uci" command by sending engine info and options.
            SendUCIInfo();
        }
        else if (token == UCI_CMD_ISREADY)
        {
            // Handle the "isready" command by indicating the engine is ready.
            std::cout << "readyok" << std::endl;
            std::cout.flush();
        }
        else if (token == UCI_CMD_POSITION)
        {
            // Handle the "position" command by setting up the board position.
            // Parse the rest of the command to set the position.
            // Example: "position startpos moves e2e4 e7e5"
            // Example: "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR moves e2e4 e7e5"
            std::string positionType;
            iss >> positionType;
            if (positionType == "startpos")
            {
                UCILog("startpos :rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
                // Handle the "startpos" command by setting up the board position to the starting position.
                FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
                ProcessMoves(iss);
            }
            else if (positionType == "fen")
            {
                // Handle the "fen" command by setting up the board position to the given FEN string.
                // Parse the rest of the command to get the FEN string.
                // Example: "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"
                std::string fen;
                iss >> fen;
                if (fen != _state)
                {
                    FENtoBoard(fen);
                    _state = fen;
                }
                ProcessMoves(iss);
            }
            else
            {
                UCILog("ERROR : " + positionType);
            }
        }
        else if (token == UCI_CMD_GO)
        {
            _instance->getCurrentPlayer()->playerNumber() == 0 ? UCILog("AI IS WHITE") : UCILog("AI IS BLACK");
            _instance->updateAI();
        }
        else if (token == UCI_CMD_STOP)
        {
            // Handle the "stop" command by stopping the engine's search.
            // Stop searching and send the best move found so far.
        }
        else if (token == UCI_CMD_MOVE)
        {
            // Handle the "move" command by processing the actual moves from the other AI.
            // Example: "move e2e4"
            std::string move;
            iss >> move;
            _instance->UCIMove(move);
            UCILog("move : " + move);
            // Process the move received from the other AI
            // Update your board state accordingly
        }
        else if (token == UCI_CMD_QUIT)
        {
            // Handle the "quit" command by gracefully exiting the program.
            exit(0);
        }
        // Add more handlers for other UCI commands as needed.
    }

    void SendUCICommand(const std::string &command)
    {
        std::cout << command << std::endl;
        std::cout.flush();
        UCILog(command);
    }

    // Function to send engine info and options.
    void SendUCIInfo()
    {
        // Send the engine's identity and available options in UCI format.
        // Example:
        std::cout << "id name Adler2023" << std::endl;
        std::cout << "id author Graeme" << std::endl;
        // Add more information and options as needed.
        std::cout << "uciok" << std::endl;
        std::cout.flush();
    }
    Chess *_instance;
    std::string _state;
};
