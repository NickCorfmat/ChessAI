#include "Application.h"
#include "imgui/imgui.h"
#include "classes/TicTacToe.h"
#include "classes/Chess.h"
#include "classes/Zobrist.h"

namespace ClassGame {
        //
        // our global variables
        //
        Chess *chess_game = nullptr;
        TicTacToe *ttt_game = nullptr;
        
        int gameWinner = -1;
        int AIPlayer = 1;
        int HumanPlayer = -1;

        bool ticTacToeSelected = false;
        bool chessSelected = false;
        bool selectedColor = false;
        bool gameOver = false;

        Zobrist zobrist;

        //
        // game starting point
        // this is called by the main render loop in main.cpp
        //
        void GameStartUp() {
    #ifndef UCI_INTERFACE
            ImGuiStyle& style = ImGui::GetStyle();
            style.Alpha = 1.0f;
            style.FrameRounding = 3.0f;
            style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
            style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

            for (int i = 0; i <= ImGuiCol_COUNT; i++) {
                ImVec4& col = style.Colors[i];
                float H, S, V;
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

                if(S < 0.1f) {
                    V = 1.0f - V;
                }
                ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
                if(col.w < 1.00f) {
                    col.w *= 0.5f;
                }
            }
    #endif
        }

        //
        // game render loop
        // this is called by the main render loop in main.cpp
        //
        void RenderGame() {
    #if defined(UCI_INTERFACE)
                if (!selectedColor) {
                    AIPlayer = 1;
                    HumanPlayer = 0;
                    selectedColor = true;
                    chess_game = new Chess();
                    chess_game->_gameOptions.AIPlayer = AIPlayer;
                    chess_game->_gameOptions.HumanPlayer = HumanPlayer;
                    chess_game->setUpBoard();
                }
                if (chess_game->gameHasAI() && chess_game->getCurrentPlayer()->isAIPlayer()) {
                    chess_game->updateAI();
                }
                static int counter = 0;
                if (counter > 2000000 && counter < 2000010) {
                    std::cout << "info string Entering Run method" << std::endl;
                }
                counter++;
                chess_game->drawFrame();
    #else
                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

                ImGui::Begin("Settings");

                if (!ticTacToeSelected && !chessSelected) {
                    if (ImGui::Button("Play TicTacToe")) {
                        ticTacToeSelected = true;
                        ttt_game = new TicTacToe();
                        ttt_game->setUpBoard();
                    }
                    if (ImGui::Button("Play Chess")) {
                        chessSelected = true;
                    }
                    ImGui::End();
                } else if (ticTacToeSelected) { // TicTacToe Game
                    ImGui::Text("Current Player Number: %d", ttt_game->getCurrentPlayer()->playerNumber());
                    ImGui::Text("Current Board State: %s", ttt_game->stateString().c_str());

                    if (gameOver) {
                        ImGui::Text("Game Over!");
                        ImGui::Text("Winner: %d", gameWinner);
                        if (ImGui::Button("Reset Game")) {
                            ttt_game->stopGame();
                            ttt_game->setUpBoard();
                            gameOver = false;
                            gameWinner = -1;
                        }
                    }
                    ImGui::End();

                    ImGui::Begin("GameWindow");
                    ttt_game->drawFrame();
                    ImGui::End();
                } else { // Chess Game
                    if (!selectedColor) {
                        if (ImGui::Button("Play WHITE")) {
                            AIPlayer = 1;
                            HumanPlayer = 0;
                            selectedColor = true;   
                            chess_game = new Chess();
                            chess_game->_gameOptions.AIPlayer = AIPlayer;
                            chess_game->_gameOptions.HumanPlayer = HumanPlayer;
                            chess_game->setUpBoard();
                        }
                        if (ImGui::Button("Play BLACK")) {
                            AIPlayer = 0;
                            HumanPlayer = 1;
                            selectedColor = true;   
                            chess_game = new Chess();
                            chess_game->_gameOptions.AIPlayer = AIPlayer;
                            chess_game->_gameOptions.HumanPlayer = HumanPlayer;
                            chess_game->setUpBoard();
                        }
                        ImGui::End();
                    } else {
                        ImGui::Text("Current Player Number: %d", chess_game->getCurrentPlayer()->playerNumber());
                        ImGui::Text("");
                        std::string state = chess_game->stateString();
                        //
                        // break state string into 8 rows of 8 characters
                        //
                        if (state.length() == 64) {
                            for (int y=0; y<8; y++) {
                                std::string row = state.substr(y*8, 8);
                                ImGui::Text("%s", row.c_str());
                            }
                            ImGui::Text("");
                            int64_t hash = zobrist.ZobristHash(state.c_str(),64);
                            ImGui::Text("zobrist hash: %llx", hash);
                            ImGui::Text("");
                            ImGui::Text("board evaluation value: %d", chess_game->evaluateBoard(state.c_str()));
                            ImGui::Text("");
                            if (chess_game->isKingInCheck(state.c_str(), 'W')) {
                                ImGui::Text("ALERT: White king is in check!");
                            } else if (chess_game->isKingInCheck(state.c_str(), 'B')) {
                                ImGui::Text("ALERT: Black king is in check!");
                            }
                        } else {
                            ImGui::Text("%s", state.c_str());
                        }
                        if (chess_game->gameHasAI()) {
                            ImGui::Text("");
                            ImGui::Text("AI Depth Searches: %d", chess_game->getAIDepathSearches());
                        }
                        if (gameOver) {
                            ImGui::Text("Game Over!");
                            ImGui::Text("%s", (std::string("Winner: ") + (gameWinner == 0 ? "White" : "Black")).c_str());
                            if (ImGui::Button("Reset Game")) {
                                chess_game->stopGame();
                                chess_game->setUpBoard();
                                gameOver = false;
                                gameWinner = -1;
                            }
                        }
                        ImGui::End();
                    }

                
                    ImGui::Begin("GameWindow");
                    if (selectedColor) {
                        if (chess_game->gameHasAI() && (chess_game->getCurrentPlayer()->playerNumber() == AIPlayer)) {
                            chess_game->updateAI();
                        }
                        chess_game->drawFrame();
                    
                    }
                    ImGui::End();
                }
    #endif
        }

        //
        // end turn is called by the game code at the end of each turn
        // this is where we check for a winner
        //
        void EndOfTurn() {
            if (ticTacToeSelected) {
                Player *winner = ttt_game->checkForWinner();
                if (winner) {
                    gameOver = true;
                    gameWinner = winner->playerNumber();
                }
                if (ttt_game->checkForDraw()) {
                    gameOver = true;
                    gameWinner = -1;
                }
            } else if (chessSelected) {
                Player *winner = chess_game->checkForWinner();

                if (winner) {
                    gameOver = true;
                    gameWinner = winner->playerNumber();
                }
                if (chess_game->checkForDraw()) {
                    gameOver = true;
                    gameWinner = -1;
                }
            }
        }
}
