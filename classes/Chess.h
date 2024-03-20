// --------------------------------------------------------------
// Nicolas Corfmat
// CruzID: ncorfmat
// Assignment: Make a Chess Game
// --------------------------------------------------------------

#pragma once
#include "Game.h"
#include "Zobrist.h"
#include "ChessSquare.h"

const int pieceSize = 100;
const int zobristSize = 0x100000;

//
// the classic game of chess
//
enum ChessPiece {
    Pawn = 1,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

struct Transposition {
    int score;
    int depth;
    int alpha;
    int beta;
    int age;
    uint64_t hash;
};

//
// the main game class
//
class Chess : public Game {
public:
    Chess();
    ~Chess();

    struct Move {
        std::string from;
        std::string to;
    };

    // set up the board
    void        setUpBoard() override;

    Player*     checkForWinner() override;
    bool        checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() const override;
    std::vector<Chess::Move> generateMovesInternal(const char *state, int& castleStatus, std::string lastMove, char color);
    std::vector<Chess::Move> generateMoves(const char *state, int& castleStatus, int& gameStatus, std::string lastMove, char color);
    void        UCIMove(const std::string &move);
    void        pieceTaken(Bit *bit);
    void        performCastling(Bit& bit, ChessSquare& src, ChessSquare& dst);
    void        addMoveIfValid(std::vector<Chess::Move>& moves, const char *state, int fromRow, int fromCol, int toRow, int toCol);
    void        generateKnightMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col);
    void        generatePawnMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, std::string lastMove, char color);
    void        generateLinearMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, const std::vector<std::pair<int, int>> directions);
    void        generateBishopMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col);
    void        generateRookMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col);
    void        generateQueenMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col);
    void        generateKingMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, int castleStatus, char color);
    char        oppositeColor(char color);
    void        setStateString(const std::string &s) override;
    bool        actionForEmptyHolder(BitHolder &holder) override;
    void        clearBoardHighlights() override;
    bool        canBitMoveFrom(Bit& bit, BitHolder& src) override;
    bool        canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
    void        bitMovedFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
    bool	    clickedBit(Bit& bit) override;
    void        stopGame() override;
    bool        isPieceColor(char piece, char color);
    bool        isSlidingPiece(char piece);
    bool        doesPieceMatch(char piece, char type);
    int         evaluateBoard(const char* stateString);
    const char  pieceNotation(const char *state, int row, int column);
    const char* indexToNotation(int row, int col);
    bool        checkForCheck(const char *state, unsigned char color, int64_t opponentMovesBitBoard);
    bool        isKingInCheck(const char* state, char kingColor);
    bool        canSquareBeAttacked(const char* state, int square, char attackerColor);
    bool        knightCanAttackSquare(const char* state, int square, char attackerColor);
    bool        slidingPiecesCanAttackSquare(const char* state, int square, char attackerColor);
    bool        adjacentPiecesCanAttackSquare(const char* state, int square, char attackerColor);
    int64_t     generateMovesBitBoard(std::vector<Chess::Move>& moves);
    unsigned int makeAIMove(char *state, int castleStatus, Chess::Move& move);
    unsigned int makeAIMove(char *state, int castleStatus, int fromIndex, int toIndex);
    void undoAIMove(char *state, unsigned int move);
    int minimaxAlphaBetaSorted(char* state, int depth, bool isMaximizingPlayer, int& castleStatus, int alpha, int beta);
    void updateAI() override;

    int             _depthSearches;
    int             _zobristHits;
    int             _zobristMisses;
    Zobrist         _zobrist;   
    static Transposition     _zobristHashes[zobristSize];

    std::pair<int, int> notationToIndex(const char* notation);

    BitHolder &getHolderAt(const int x, const int y) override { return _grid[y][x]; } 

    int             _castleStatus;
    int             gameStatus;

    std::vector<Chess::Move> _moves;
    std::vector<Chess::Move> _opponentMoves;

    uint64_t _movesBitBoard;
    uint64_t _opponentMovesBitBoard;

    Bit *       PieceForPlayer(const int playerNumber, ChessPiece piece);

private:
    const char  bitToPieceNotation(int row, int column) const;
    
    ChessSquare         _grid[8][8];
    Player* _winner;
};
