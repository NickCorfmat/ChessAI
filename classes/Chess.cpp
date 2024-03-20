// --------------------------------------------------------------
// Nicolas Corfmat
// CruzID: ncorfmat
// Assignment: Make a Chess Game
// --------------------------------------------------------------

// USAGE: 
// Run 'make' on the command line followed by ./gameboard
// Run 'make clean' to remove all generated files from chessEmpty directory

#include "Chess.h"
#include "UCI.h"

#define ZOBRISTHASH 1

Transposition Chess::_zobristHashes[zobristSize];

/*** Global Variables ***/

UCIInterface uciInterface;

/*** Constants ***/
const int DEPTH_LIMIT = 20;

const int Playing = 0;
const int Check = 1;
const int Checkmate = 2;

/*** Boolean Flags */

bool performedEnPassant = false;

bool whiteKingMoved = false;
bool whiteRookLMoved = false;
bool whiteRookRMoved = false;
bool blackKingMoved = false;
bool blackRookLMoved = false;
bool blackRookRMoved = false;

/*** Constructor & Destructor ***/

Chess::Chess() {
    Transposition zero;
    zero.age = 0;
    zero.alpha = 0;
    zero.beta = 0;
    zero.depth = 0;
    zero.score = 0;
    zero.hash = 0;

    for (int i = 0; i < (zobristSize-1); i++) {
        _zobristHashes[i] = zero;
    }
}

Chess::~Chess() { }

/*** Game Functions ***/

// MAKE a chess piece for a player
Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece) {
    const char *pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };
    Bit *bit = new Bit();
    const char *pieceName = pieces[piece - 1];
    std::string spritePath = std::string("chess/") + (playerNumber == 1 ? "b_" : "w_") + pieceName; // depending on playerNumber load the black or white chess piece graphic
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    return bit;
}

// SCAN for player input
void Game::scanForMouse() {
    if (gameHasAI() && (getCurrentPlayer()->playerNumber() == _gameOptions.AIPlayer)) {
        updateAI();
        return;
    }

    ImVec2 mousePos = ImGui::GetMousePos();
    mousePos.x -= ImGui::GetWindowPos().x;
    mousePos.y -= ImGui::GetWindowPos().y;

	Entity *entity = nullptr;
	for (int y = 0; y < _gameOptions.rowY; y++) {
		for (int x = 0; x < _gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
			Bit *bit = holder.bit();
			if (bit && bit->isMouseOver(mousePos)) {
				entity = bit;
			} else if (holder.isMouseOver(mousePos)) {
				entity = &holder;
			}
		}
	}    
	if (ImGui::IsMouseClicked(0)) {
		mouseDown(mousePos, entity);
	} else if (ImGui::IsMouseReleased(0)) {
		mouseUp(mousePos, entity);
	} else {
		mouseMoved(mousePos, entity);
	}
}

// GENERATE game frame
void Game::drawFrame() {
    scanForMouse();

    for (int y=0; y<_gameOptions.rowY; y++) {
        for (int x=0; x<_gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
            holder.paintSprite();
		}
    }
	// paint the pieces second so they are always on top of the board as we move them
    for (int y=0; y<_gameOptions.rowY; y++) {
        for (int x=0; x<_gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
            if (holder.bit() && !holder.bit()->getPickedUp()) {
                holder.bit()->paintSprite();
            }
        }
    }
	// now paint any picked up pieces
    for (int y=0; y<_gameOptions.rowY; y++) {
        for (int x=0; x<_gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
            if (holder.bit() && holder.bit()->getPickedUp()) {
                holder.bit()->paintSprite();
            }
        }
    }

    // animate moves
    for (int y=0; y<_gameOptions.rowY; y++) {
        for (int x=0; x<_gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
            if (holder.bit() && holder.bit()->isMoving()) {
                holder.bit()->animateMove();
                holder.bit()->paintSprite();
            }
        }
    }
}

void Chess::UCIMove(const std::string &move) {
    int fromCol = move[0] - 'a';
    int fromRow = move[1] - '1';
    int toCol = move[2] - 'a';
    int toRow = move[3] - '1';
    BitHolder &src = getHolderAt(fromCol, fromRow);
    BitHolder &dst = getHolderAt(toCol, toRow);
    Bit *bit = src.bit();
    if (dst.bit())
    {
        pieceTaken(dst.bit());
    }
    dst.dropBitAtPoint(bit, ImVec2(0, 0));
    src.setBit(nullptr);
    // this also calls endTurn
    bitMovedFromTo(*bit, src, dst);
    uciInterface.UCILog("Chess::UCIMove: " + move);
}

void Chess::pieceTaken(Bit *bit) {
    // Add code to play a sound when piece is clicked
}

// HELPER that returns FEN chess notation in the form p for white pawn, K for black king, etc.
const char Chess::bitToPieceNotation(int row, int column) const {
    if (row < 0 || row >= 8 || column < 0 || column >= 8) {
        return '0';
    }
    
    const char *bpieces = { "?PNBRQK" };
    const char *wpieces = { "?pnbrqk" };
    unsigned char notation = '0';
    Bit *bit = _grid[row][column].bit();
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()&127];
    } else {
        notation = '0';
    }
    return notation;
}

// CONVERT row and column to standard chess notation
const char* Chess::indexToNotation(int row, int col) {
    static char notation[3];
    notation[0] = 'a' + col;
    notation[1] = '8' - row;
    notation[2] = 0;
    return notation;
}

// HELPER to retrieve piece notation at (row, col), i.e 'Q'
const char Chess::pieceNotation(const char *state, int row, int column) {
    if (row < 0 || row >= 8 || column < 0 || column >= 8) {
        return '0';
    }
    return state[row*8 + column];
}

// INITIALIZES the default chess board state
void Chess::setUpBoard() {
    const ChessPiece initialBoard[2][8] = {
        {Rook, Knight, Bishop, Queen, King, Bishop, Knight, Rook},  // 1st Rank
        {Pawn, Pawn, Pawn, Pawn, Pawn, Pawn, Pawn, Pawn}  // 2nd Rank
    };

    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            _grid[y][x].initHolder(ImVec2((float)(100*x + 100),(float)(100*y + 100)), "boardsquare.png", x, y);
            _grid[y][x].setGameTag(0);
            _grid[y][x].setNotation(indexToNotation(y, x));
        }
    }

    for (int rank = 0; rank < 2; rank++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            ChessPiece piece = initialBoard[rank][x];
            
            // Top side
            Bit *bit = PieceForPlayer(_gameOptions.AIPlayer, initialBoard[rank][x]);
            bit->setPosition(_grid[rank][x].getPosition());
            bit->setParent(&_grid[rank][x]);
            bit->setGameTag((_gameOptions.AIPlayer == 1) ? piece + 128 : piece);
            bit->setSize(pieceSize, pieceSize);
            _grid[rank][x].setBit(bit);

            // Bottom side
            bit = PieceForPlayer(_gameOptions.HumanPlayer, initialBoard[rank][x]);
            bit->setPosition(_grid[7-rank][x].getPosition());
            bit->setParent(&_grid[7-rank][x]);
            bit->setGameTag((_gameOptions.HumanPlayer == 1) ? piece + 128 : piece);
            bit->setSize(pieceSize, pieceSize);
            _grid[7-rank][x].setBit(bit);
        }
    }

    gameStatus = Playing;
    _winner = nullptr;

    if (gameHasAI()) {
        setAIPlayer(_gameOptions.AIPlayer);
    }

    startGame();

#if defined(UCI_INTERFACE)
    uciInterface.Run(this);
#endif

}

char Chess::oppositeColor(char color) {
    return color == 'W' ? 'B' : 'W';
}

bool Chess::clickedBit(Bit& bit) {
    clearBoardHighlights();
    return true;
}

bool Chess::actionForEmptyHolder(BitHolder &holder) {
    return false;
}

// UN-HIGHLIGHTS all highlighted squares
void Chess::clearBoardHighlights() {
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            ChessSquare &dstSquare = _grid[y][x];
            dstSquare.setMoveHighlighted(false);
        }
    }
}

// APPENDS a valid move to a vector array of all possible moves
void Chess::addMoveIfValid(std::vector<Move>& moves, const char *state, int fromRow, int fromCol, int toRow, int toCol) {
    if (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
        unsigned char pieceA = pieceNotation(state, fromRow, fromCol);
        unsigned char pieceAColor = (pieceA >= 'a' && pieceA <= 'z') ? 'W' : (pieceA >= 'A' && pieceA <= 'Z') ? 'B' : ' ';
        unsigned char pieceB = pieceNotation(state, toRow, toCol);
        unsigned char pieceBColor = (pieceB >= 'a' && pieceA <= 'z') ? 'W' : (pieceB >= 'A' && pieceB <= 'Z') ? 'B' : ' ';

        if (pieceAColor != pieceBColor) {
            // Create a copy of the current chessboard state
            char stateCopy[65];
            strcpy(stateCopy, state);
            stateCopy[toRow*8 + toCol] = stateCopy[fromRow*8 + fromCol];
            stateCopy[fromRow*8 + fromCol] = '0';

            if (!isKingInCheck(stateCopy, pieceAColor)) {
                moves.push_back({indexToNotation(fromRow, fromCol), indexToNotation(toRow, toCol)}); // Append every move that doesn't put current player's king in check
            }
        }
    }
}

// CONVERTS standard chess notation to its (row, col) index, ex 'a5' --> (3, 0)
std::pair<int, int> Chess::notationToIndex(const char* notation) {
    int col = notation[0] - 'a';
    int row = '8' - notation[1];

    return std::make_pair(row, col);
}

// CHECK if piece can move from source square
bool Chess::canBitMoveFrom(Bit& bit, BitHolder& src) {
    if ((bit.gameTag() < 128 ? 'W' : 'B') != ((getCurrentPlayer()->playerNumber() == 0) ? 'W' : 'B')) {
        return false; // Immediately exit if the selected bit doesn't belong to the current player
    }

    ChessSquare &srcSquare = static_cast<ChessSquare&>(src);
    
    char color = (_gameOptions.HumanPlayer == 0) ? 'W' : 'B';
    std::string const stateStr = stateString();
    _moves = generateMoves(stateStr.c_str(), _castleStatus, gameStatus, _lastMove, color);
    bool canMove = false;

    // Highlight all moves from the current player's moveset which our selected piece can move to
    for (auto move : _moves) {
        if (move.from == srcSquare.getNotation()) {
            canMove = true;

            for (int y = 0; y < _gameOptions.rowY; y++) {
                for (int x = 0; x < _gameOptions.rowX; x++) {
                    ChessSquare &dstSquare = _grid[y][x];

                    if (move.to == dstSquare.getNotation()) {
                        dstSquare.setMoveHighlighted(true);
                    }
                }
            }
        }
    }

    return canMove;
}

// CHECK if piece can move from source to destination square
bool Chess::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
    ChessSquare &srcSquare = static_cast<ChessSquare&>(src);
    ChessSquare &dstSquare = static_cast<ChessSquare&>(dst);

    for (auto move: _moves) {
        if (move.from == srcSquare.getNotation() && move.to == dstSquare.getNotation()) {
            return true;
        }
    }

    return false;
}

// PERFORMS either kingside or queenside castling depending on which move was played
void Chess::performCastling(Bit& bit, ChessSquare& src, ChessSquare& dst) {    
    int distance = src.getDistance(dst);

    if (((bit.gameTag()&127) == ChessPiece::King) && (distance == 2)) { // Check if king moved two squares
        int rookSrcCol = (dst.getColumn() == 6) ? 7 : 0;
        int rookDstCol = (dst.getColumn() == 6) ? 5 : 3;

        BitHolder &rookSrc = getHolderAt(rookSrcCol, dst.getRow());
        BitHolder &rookDst = getHolderAt(rookDstCol, dst.getRow());
        Bit *rook = rookSrc.bit();

        rookDst.setBit(rook);
        rookSrc.setBit(nullptr);
        rook->setPosition(rookDst.getPosition());
    }
}

// UPDATES the gameboard to its new state after previous move was played
void Chess::bitMovedFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
    const char *bpieces = "pnbrqk";
    const char *wpieces = "PNBRQK";

    ChessSquare &srcSquare = static_cast<ChessSquare&>(src);
    ChessSquare &dstSquare = static_cast<ChessSquare&>(dst);

    _lastMove = "x-" + srcSquare.getNotation() + "-" + dstSquare.getNotation(); // Converts the previous move into chess notation. Ex: "x-e1-g1"
    std::string lastMoveSrc = _lastMove.substr(2, 2);

    if (lastMoveSrc == "e1" && dstSquare.getColumn() == 6) {  // White king castling kingside
        if (!whiteKingMoved && !whiteRookRMoved) {
            performCastling(bit, srcSquare, dstSquare); // Perform castling
            // Update flags
            whiteKingMoved = true;
            whiteRookRMoved = true;
        }
    } else if (lastMoveSrc == "e1" && dstSquare.getColumn() == 2) {  // White king castling queenside
        if (!whiteKingMoved && !whiteRookLMoved) {
            performCastling(bit, srcSquare, dstSquare); // Perform castling
            // Update flags
            whiteKingMoved = true;
            whiteRookLMoved = true;
        }
    } else if (lastMoveSrc == "e8" && dstSquare.getColumn() == 6) {  // Black king castling kingside
        if (!blackKingMoved && !blackRookRMoved) {
            performCastling(bit, srcSquare, dstSquare); // Perform castling
            // Update flags
            blackKingMoved = true;
            blackRookRMoved = true;
        }
    } else if (lastMoveSrc == "e8" && dstSquare.getColumn() == 2) {  // Black king castling queenside
        if (!blackKingMoved && !blackRookLMoved) {
            performCastling(bit, srcSquare, dstSquare); // Perform castling
            // Update flags
            blackKingMoved = true;
            blackRookLMoved = true;
        }
    } else {
        // Check for other piece movements and update flags
        if (lastMoveSrc == "e1" && (bit.gameTag() < 128)) { // white king
            whiteKingMoved = true;
        }
        if (lastMoveSrc == "h1" && (bit.gameTag() < 128)) { // white rook kingside
            whiteRookRMoved = true;
        }
        if (lastMoveSrc == "a1" && (bit.gameTag() < 128)) { // white rook queenside
            whiteRookLMoved = true;
        }
        if (lastMoveSrc == "e8" && (bit.gameTag() >= 128)) { // black king
            blackKingMoved = true;
        }
        if (lastMoveSrc == "h8" && (bit.gameTag() >= 128)) { // black rook kingside
            blackRookRMoved = true;
        }
        if (lastMoveSrc == "a8" && (bit.gameTag() >= 128)) { // black rook queenside
            blackRookLMoved = true;
        }
    }

    // En passant
    if (performedEnPassant) {
        int direction = (bit.getOwner()->playerNumber() == _gameOptions.HumanPlayer) ? 1 : -1;
        BitHolder &enPassantSquare = getHolderAt(dstSquare.getColumn(), dstSquare.getRow() + direction);
        Bit *enPassantBit = enPassantSquare.bit();

        if (enPassantBit) {
            enPassantSquare.setBit(nullptr);
        }

        performedEnPassant = false;
    }

    // Promotion
    if ((bit.gameTag()&127) == ChessPiece::Pawn) {
        int promotionRow = (bit.getOwner()->playerNumber() == _gameOptions.HumanPlayer) ? 0 : 7;

        if (dstSquare.getRow() == promotionRow) {            
            BitHolder &promotionSquare = getHolderAt(dstSquare.getColumn(), promotionRow);
            Bit *promotionPiece = PieceForPlayer(bit.getOwner()->playerNumber(), Queen);

            if (promotionPiece) {
                promotionPiece->setPosition(dstSquare.getPosition());
                promotionPiece->setParent(&dstSquare);
                promotionPiece->setGameTag((bit.getOwner()->playerNumber() == 0) ? Queen : Queen + 128);
                promotionPiece->setSize(pieceSize, pieceSize);
                promotionSquare.setBit(promotionPiece);
            }
        }
    }

    Game::bitMovedFromTo(bit, src, dst);
}

/*** Generate Moves ***/

// GENERATES both current player and opponents moves, and checks if our king is currently in check. If so,
// only allow player to move a piece that would exit the check. This function also detects checkmate.
std::vector<Chess::Move> Chess::generateMoves(const char *state, int& castleStatus, int& gameStatus, std::string lastMove, char color) {
    gameStatus = Playing;

    std::vector<Chess::Move> moves = generateMovesInternal(state, castleStatus, lastMove, color);
    std::vector<Move> oppMoves = generateMovesInternal(state, castleStatus, lastMove, oppositeColor(color));

    bool inCheck = checkForCheck(state, color, generateMovesBitBoard(oppMoves));

    // If we're in check we can only allow moves that get us out of check

    if (inCheck) {
        std::vector<Move> movesInCheck;
        gameStatus = Check;

        for (auto move : moves) {
            char newState[65];
            strcpy(newState, state);
            int fromRow = 7 - (move.from[1] - '1');
            int fromCol = move.from[0] - 'a';
            int toRow = 7 - (move.to[1] - '1');
            int toCol = move.to[0] - 'a';
            newState[toRow*8 + toCol] = newState[fromRow*8 + fromCol];
            newState[fromRow*8 + fromCol] = '0';
            std::vector oppMoves = generateMovesInternal(newState, castleStatus, lastMove, oppositeColor(color));
            
            if (!checkForCheck(newState, color, generateMovesBitBoard(oppMoves))) {
                movesInCheck.push_back(move);
            }
        }

        if (movesInCheck.size() == 0) {
            gameStatus = Checkmate;
        }

        return movesInCheck;
    }

    return moves;
}

// GENERATES a vector list of all moves
std::vector<Chess::Move> Chess::generateMovesInternal(const char *state, int& castleStatus, std::string lastMove, char color) {
    std::vector<Chess::Move> moves;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = pieceNotation(state, row, col);
            if (piece != '0') {
                bool correctColor = false;
                if (color == 'W') {
                    correctColor = piece >= 'a' && piece <= 'z';
                } else if (color == 'B') {
                    correctColor = piece >= 'A' && piece <= 'Z';
                }

                if (correctColor) {
                    piece = toupper(piece);
                    switch (toupper(piece)) {
                        case 'N':
                            generateKnightMoves(moves, state, row, col);
                            break;
                        case 'P':
                            generatePawnMoves(moves, state, row, col, lastMove, color);
                            break;
                        case 'B':
                            generateBishopMoves(moves, state, row, col);
                            break;
                        case 'R':
                            generateRookMoves(moves, state, row, col);
                            break;
                        case 'K':
                            generateKingMoves(moves, state, row, col, castleStatus, color);
                            break;
                        case 'Q':
                            generateQueenMoves(moves, state, row, col);
                            break;
                    }
                }
            }
        }
    }

    return moves;
}

// GENERATES linear paths for linear-traveling pieces
void Chess::generateLinearMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, const std::vector<std::pair<int, int>> directions) {
    for (auto& dir : directions) {
        int currentRow = row + dir.first;
        int currentCol = col + dir.second;

        while (currentRow >= 0 && currentRow < 8 && currentCol >= 0 && currentCol < 8) {
            if (pieceNotation(state, currentRow, currentCol) != '0') {
                addMoveIfValid(moves, state, row, col, currentRow, currentCol);
                break;
            }

            addMoveIfValid(moves, state, row, col, currentRow, currentCol);
            currentRow += dir.first;
            currentCol += dir.second;
        }
    }
}

// GENERATES valid Pawn moves
void Chess::generatePawnMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, std::string lastMove, char color) {
    int direction = (color == (_gameOptions.HumanPlayer == 0 ? 'W' : 'B')) ? -1 : 1;
    int startRow = (color == (_gameOptions.HumanPlayer == 0 ? 'W' : 'B')) ? 6 : 1;

    // Generate forward moves
    if (pieceNotation(state, row + direction, col) == '0') {
        addMoveIfValid(moves, state, row, col, row + direction, col); // Possible 1-space forward move

        if (row == startRow && pieceNotation(state, row + (2 * direction), col) == '0') {
            addMoveIfValid(moves, state, row, col, row + (2 * direction), col); // Possible 2-space forward move
        }
    }

    // Generate diagonal moves
    int newRow = row + direction;

    if (newRow >= 0 && newRow < 8 && (col + 1) >= 0 && (col + 1) < 8) {  // Right diagonal
        if (pieceNotation(state, newRow, col + 1) != '0') {
            addMoveIfValid(moves, state, row, col, newRow, col + 1);
        }
    }
    if (newRow >= 0 && newRow < 8 && (col - 1) >= 0 && (col - 1) < 8) {  // Left diagonal
        if (pieceNotation(state, newRow, col - 1) != '0') {
            addMoveIfValid(moves, state, row, col, newRow, col - 1);
        }
    }

    // En passant
    char lastMovePiece = lastMove[0];
    int lastMoveStartRow = lastMove[3] - '0';
    int lastMoveEndRow = lastMove[6] - '0';
    int lastMoveStartCol = lastMove[2] - 'a';
    int lastMoveEndCol = lastMove[5] - 'a';

    if (color == 'W' && row == 3) {
        if (lastMovePiece == 'p' || lastMoveStartRow == 7 && lastMoveEndRow == 5) {
            if (lastMoveEndCol == col - 1 || lastMoveEndCol == col + 1) {
                addMoveIfValid(moves, state, row, col, row - 1, lastMoveEndCol);
                performedEnPassant = true;
            }
        }
    } else if (color == 'B' && row == 4) {
        if (lastMovePiece == 'P' || lastMoveStartRow == 2 && lastMoveEndRow == 4) {
            if (lastMoveEndCol == col - 1 || lastMoveEndCol == col + 1) {
                addMoveIfValid(moves, state, row, col, row + 1, lastMoveEndCol);
                performedEnPassant = true;
            }
        }
    }
}

// GENERATES valid Knight moves
void Chess::generateKnightMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col) {
    static const int movesRow[] = {2, 1, -1, -2, -2, -1, 1, 2};
    static const int movesCol[] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < 8; i++) {
        int newRow = row + movesRow[i];
        int newCol = col + movesCol[i];
        addMoveIfValid(moves, state, row, col, newRow, newCol);
    }
}

// GENERATES valid Bishop moves
void Chess::generateBishopMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col) {
    static const std::vector<std::pair<int, int>> directions = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // Diagonal moves
    generateLinearMoves(moves, state, row, col, directions); // Compute diagonal moves
}

// GENERATES valid Rook moves
void Chess::generateRookMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col) {
    static const std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; // Vertical and horizontal moves
    generateLinearMoves(moves, state, row, col, directions); // Compute vertical and horizontal moves
}

// GENERATES valid Queen moves
void Chess::generateQueenMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col) {
    generateRookMoves(moves, state, row, col); // Compute vertical and horizontal moves
    generateBishopMoves(moves, state, row, col); // Compute diagonal moves
}

// GENERATES valid King moves
void Chess::generateKingMoves(std::vector<Chess::Move>& moves, const char *state, int row, int col, int castleStatus, char color) {
    static const std::vector<std::pair<int, int>> directions = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} // All 8 directions
    };

    // Generate all 8 adjacent locations
    for (int i = 0; i < 8; i++) {
        int newRow = row + directions[i].first;
        int newCol = col + directions[i].second;
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            addMoveIfValid(moves, state, row, col, newRow, newCol);
        }
    }

    // Castling
    if (color == 'W') {
        if (!whiteKingMoved && !whiteRookLMoved) {
            if (pieceNotation(state, row, col - 1) == '0' && pieceNotation(state, row, col - 2) == '0' && pieceNotation(state, row, col - 3) == '0'
            && pieceNotation(state, row, col - 4 ) == (color == 'W' ? 'r' : 'R')) {
                addMoveIfValid(moves, state, row, col, row, col - 2); // White king can castle queenside
            }
        }
        if (!whiteKingMoved && !whiteRookRMoved) {
            if (pieceNotation(state, row, col + 1) == '0' && pieceNotation(state, row, col + 2) == '0' && pieceNotation(state, row, col + 3) == (color == 'W' ? 'r' : 'R')) {
                addMoveIfValid(moves, state, row, col, row, col + 2); // White king can castle kingside
            }
        }
    } else {
        if (!blackKingMoved && !blackRookLMoved) {
            if (pieceNotation(state, row, col - 1) == '0' && pieceNotation(state, row, col - 2) == '0' && pieceNotation(state, row, col - 3) == '0'
            && pieceNotation(state, row, col - 4 ) == (color == 'W' ? 'r' : 'R')) {
                addMoveIfValid(moves, state, row, col, row, col - 2); // Black king can castle queenside
            }
        }
        if (!blackKingMoved && !blackRookRMoved) {
            if (pieceNotation(state, row, col + 1) == '0' && pieceNotation(state, row, col + 2) == '0' && pieceNotation(state, row, col + 3) == (color == 'W' ? 'r' : 'R')) {
                addMoveIfValid(moves, state, row, col, row, col + 2); // Black king can castle kingside
            }
        }
    }
}

// GENERATES a 64-bit int representing the chessboard, with 1 indicating locations where the enemy can move
int64_t Chess::generateMovesBitBoard(std::vector<Chess::Move>& moves) {
    //generate bitboard of destination moves
    int64_t bitBoard = 0;
    for (auto move : moves) {
        int toRow = 7 - (move.to[1] - '1');
        int toCol = move.to[0] - 'a';
        bitBoard |= (1LL << (toRow*8+toCol));
    }
    return bitBoard;
}

/*** Check & Checkmate Functions ***/

// CHECKS if the current player's king is threatened by an enemy moves
bool Chess::checkForCheck(const char *state, unsigned char color, int64_t opponentMovesBitBoard) {
    // Find the king
    int kingIndex = -1;

    for (int i = 0; i < 64; i++) {
        if (state[i] == ((color == 'W') ? 'k' : 'K')) {
            kingIndex = i;
            break;
        }
    }

    if (kingIndex == -1) {
        return false;
    }

    // Check to see if any off the opponent moves are on the king
    // Create bitboard for king posiiton
    int64_t kingBit = 1LL << kingIndex;
    return (opponentMovesBitBoard & kingBit) != 0;
}

// DETECTS if the current player's king is in check. This function is mainly used before adding a possible move to our current moveset
bool Chess::isKingInCheck(const char* state, char kingColor) {
    // Find the king's position
    int kingPos = -1;

    for (int i = 0; i < 64; i++) {
        if ((kingColor == 'W' && state[i] == 'k') || (kingColor == 'B' && state[i] == 'K')) {
            kingPos = i;
            break;
        }
    }

    if (kingPos == -1) {
        return false; // Error: King not found on the board
    }
    
    return canSquareBeAttacked(state, kingPos, (kingColor == 'W' ? 'B' : 'W'));
}

// CHECKS if the given square can be attacked by opponent pieces
bool Chess::canSquareBeAttacked(const char* state, int square, char attackerColor) {
    // Check for attack along ranks, files, and diagonals (bishops, rooks, queens)
    if (slidingPiecesCanAttackSquare(state, square, attackerColor) || adjacentPiecesCanAttackSquare(state, square, attackerColor)
        || knightCanAttackSquare(state, square, attackerColor)) {
        return true;
    }

    return false;
}

// CHECKS if the given square can be attacked by enemy pawns or king
bool Chess::adjacentPiecesCanAttackSquare(const char* state, int square, char attackerColor) {
    int rank = square / 8;
    int file = square % 8;
    
    // Directions for pawns/king
    std::pair<int, int> adjacentDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    // Lambda to check if a square contains a piece that can attack
    auto pieceCanAttack = [this, &state, attackerColor](int targetRank, int targetFile, char piece) {
        if (targetRank < 0 || targetRank > 7 || targetFile < 0 || targetFile > 7) {
            return false;
        }

        char targetPiece = state[targetRank * 8 + targetFile];
        return (targetPiece != '0' && isPieceColor(targetPiece, attackerColor) && doesPieceMatch(targetPiece, piece));
    };

    for (auto& dir : adjacentDirections) {
        int newRank = rank + dir.first;
        int newFile = file + dir.second;

        if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
            if (state[newRank * 8 + newFile] != '0') {
                if (pieceCanAttack(newRank, newFile, 'k')) {
                    return true; // Adjacent opponent king found
                } else if (pieceCanAttack(newRank, newFile, 'p')) {
                    // Ensures enemy pawns can only come from a certain direction depending on the current player's color
                    if (dir.second != 0 && dir.first == (attackerColor == 'W' ? 1 : -1)) {
                        return true; // Adjacent opponent pawn(s) found
                    }
                }
            }
        }
    }

    return false;
}

// CHECKS if the given square can be attacked by enemy rooks, bishops, or queen
bool Chess::slidingPiecesCanAttackSquare(const char* state, int square, char attackerColor) {
    int rank = square / 8;
    int file = square % 8;

    // Directions for rook/queen
    std::pair<int, int> rookDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    // Directions for bishop/queen
    std::pair<int, int> bishopDirections[] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    // Lambda to check if a square contains a piece that can attack
    auto pieceCanAttack = [this, &state, attackerColor](int targetRank, int targetFile, char piece) {
        if (targetRank < 0 || targetRank > 7 || targetFile < 0 || targetFile > 7) {
            return false;
        }

        char targetPiece = state[targetRank * 8 + targetFile];
        return (targetPiece != '0' && isPieceColor(targetPiece, attackerColor) && isSlidingPiece(targetPiece) && doesPieceMatch(targetPiece, piece));
    };

    // Check for rooks or queens along ranks and files
    for (auto& dir : rookDirections) {
        for (int r = rank + dir.first, f = file + dir.second; r >= 0 && r < 8 && f >= 0 && f < 8; r += dir.first, f += dir.second) {
            if (state[r * 8 + f] != '0') {
                if (pieceCanAttack(r, f, 'r') || pieceCanAttack(r, f, 'q')) {
                    return true;
                }
                break; // Blocked by another piece
            }
        }
    }

    // Check for bishops or queens along diagonals
    for (auto& dir : bishopDirections) {
        for (int r = rank + dir.first, f = file + dir.second; r >= 0 && r < 8 && f >= 0 && f < 8; r += dir.first, f += dir.second) {
            if (state[r * 8 + f] != '0') {
                if (pieceCanAttack(r, f, 'b') || pieceCanAttack(r, f, 'q')) {
                    return true;
                }
                break; // Blocked by another piece
            }
        }
    }

    return false;
}

// CHECKS if the given square can be attacked by enemy knights
bool Chess::knightCanAttackSquare(const char* state, int square, char attackerColor) {
    int rank = square / 8;
    int file = square % 8;

    // All possible knight moves relative to the current position
    std::pair<int, int> knightMoves[] = {
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2},
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1}
    };

    for (auto& move : knightMoves) {
        int targetRank = rank + move.first;
        int targetFile = file + move.second;

        // Check if the target square is within bounds
        if (targetRank >= 0 && targetRank < 8 && targetFile >= 0 && targetFile < 8) {
            char piece = state[targetRank * 8 + targetFile];
            // Check if the piece is a knight of the attacking color

            if (isPieceColor(piece, attackerColor) && tolower(piece) == 'n') {
                return true;
            }
        }
    }

    return false;
}

// HELPER to check if given piece is of the specified color
bool Chess::isPieceColor(char piece, char color) {
    return (color == 'W') ? islower(piece) : (color == 'B') ? isupper(piece) : false;
}

// HELPER to check if given piece is either a bishop, rook, or queen
bool Chess::isSlidingPiece(char piece) {
    piece = tolower(piece);
    return piece == 'b' || piece == 'r' || piece == 'q';
}

// HELPER to check if given piece is of the specified piece type
bool Chess::doesPieceMatch(char piece, char type) {
    piece = tolower(piece);
    type = tolower(type);
    return piece == type;
}

/*** Game State Functions ***/

// FREE all the memory used by the game on the heap
void Chess::stopGame() {
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            _grid[y][x].destroyBit();
        }
    }
}

Player* Chess::checkForWinner() {
    std::vector<Chess::Move> currentMoves = generateMoves(stateString().c_str(), _castleStatus, gameStatus, _lastMove, (getCurrentPlayer()->playerNumber() == 0 ? 'W' : 'B'));

    // Check if current player can make any moves
    if (gameStatus == Checkmate && currentMoves.size() == 0) {
        _winner = getPlayerAt((getCurrentPlayer()->playerNumber() == 0 ? 1 : 0)); // Winner is other player
        return _winner;
    }

    return nullptr;
}

bool Chess::checkForDraw() {
    std::string state = stateString();

    bool isWhiteKingInCheck = isKingInCheck(state.c_str(), 'W');
    bool isBlackKingInCheck = isKingInCheck(state.c_str(), 'B');

    // Check for stalemate
    if (isWhiteKingInCheck && isBlackKingInCheck) {
        return true;
    }

    return false;
}

/*** State String ***/

std::string Chess::initialStateString() {
    return stateString();
}

std::string Chess::stateString() const {
    std::string s;
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            s += bitToPieceNotation(y, x);
        }
    }
    return s;
}

void Chess::setStateString(const std::string &s) {
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            int index = y * _gameOptions.rowX + x;
            int playerNumber = s[index] - '0';
            if (playerNumber) {
                _grid[y][x].setBit(PieceForPlayer(playerNumber - 1, Pawn));
            } else {
                _grid[y][x].setBit(nullptr);
            }
        }
    }
}

/*** Chess AI Functions ***/

int Chess::evaluateBoard(const char* stateString) {

    // piece square tables for every piece (from chess programming wiki)
    const int pawnTable[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
    };
    const int knightTable[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
     };
    const int rookTable[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
    };
    const int queenTable[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
    };
    const int kingTable[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
    };
    const int bishopTable[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
    };

    int score = 0;

    for (int i = 0; i < 64; i++) {
        // reverse x/y for white scoring
        int blackI = (7 - (i/8))*8 + (7-(i%8));

        switch(stateString[i]) {
            case 'N': // Knight
                score += 30;
                score += knightTable[blackI];
                break;
            case 'P': // Pawn
                score += 10;
                score += pawnTable[blackI];
                break;
            case 'B': // Bishop
                score += 30;
                score += bishopTable[blackI];
                break;
            case 'R': // Rook
                score += 50;
                score += rookTable[blackI];
                break;
            case 'Q': // Queen
                score += 90;
                score += queenTable[blackI];
                break;
            case 'K': // King
                score += 900;
                score += kingTable[blackI];
                break;
            case 'n': // Knight
                score -= 30;
                score -= knightTable[i];
                break;
            case 'p': // Pawn
                score -= 10;
                score -= pawnTable[i];
                break;
            case 'b': // Bishop
                score -= 30;
                score -= bishopTable[i];
                break;
            case 'r': // Rook
                score -= 50;
                score -= rookTable[i];
                break;
            case 'q': // Queen
                score -= 90;
                score -= queenTable[i];
                break;
            case 'k': // King
                score -= 900;
                score -= kingTable[i];
                break;
        }
    }

    return score;
}

void Chess::updateAI() {
    bool isMaximizingPlayer = _gameOptions.AIPlayer == 1 ? true : false;
    std::string stateString = Chess::stateString();
    char state[65];
    strcpy(state, stateString.c_str());

    // Generate moves, black is maximizing player
    std::vector<Move> possibleMoves = generateMoves(state, _castleStatus, gameStatus, _lastMove, (_gameOptions.AIPlayer == 1 ? 'B' : 'W'));
    std::vector<std::pair<int, std::pair<int, int>>> moves; // Store score and move

    int castleStatus = _castleStatus;

    for (auto move : possibleMoves) {

        unsigned int aimove = makeAIMove(state, castleStatus, move);
        int moveScore = evaluateBoard(state); // Evaluate the move with a basic score function
        int fromIndex = (aimove >> 24) & 0xFF;
        int toIndex = (aimove >> 16) & 0xFF;
        moves.push_back({moveScore, {fromIndex, toIndex}});

        // Undo move
        undoAIMove(state, aimove);
    }

    // Sort moves: if isMaximizingPlayer, sort descending. Otherwise, sort ascending
    std::sort(moves.rbegin(), moves.rend());
    int bestVal = -1000000;
    int bestMove = -1;

    for (auto& move : moves) {
        _depthSearches = 0;
        auto [fromIndex, toIndex] = move.second;
        int aimove = makeAIMove(state, castleStatus, fromIndex, toIndex);
        int value = minimaxAlphaBetaSorted(state, 0, !isMaximizingPlayer, castleStatus, 1000000, -1000000);
        _gameOptions.AIDepthSearches += _depthSearches;
        
        // If the value of the current move is more than the best value, update best

        if (value > bestVal) {
            bestMove = aimove;
            bestVal = value;
        }

        undoAIMove(state, aimove);
    }

    // Make the best move
    if (bestMove >= 0) {
        int fromIndex = (bestMove >> 24) & 0xFF;
        int toIndex = (bestMove >> 16) & 0xFF;
        int fromRow = fromIndex / 8;
        int fromCol = fromIndex % 8;
        int toRow = toIndex / 8;
        int toCol = toIndex % 8;
        
#if defined(UCI_INTERFACE)
        std::string bestUCIMove = indexToNotation(fromRow, fromCol);
        bestUCIMove += std::string(indexToNotation(toRow, toCol));
        uciInterface.UCILog("Best Move: " + bestUCIMove);
        uciInterface.SendMove(bestUCIMove);
        UCIMove(bestUCIMove);
#else
        BitHolder &src = getHolderAt(fromCol, fromRow);
        BitHolder &dst = getHolderAt(toCol, toRow);
        Bit *bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0,0));
        bit->setMoving(src.getPosition(), dst.getPosition());
        bitMovedFromTo(*bit, src, dst);
#endif
    }
}

unsigned int Chess::makeAIMove(char *state, int castleStatus, Chess::Move& move) {
    int fromRow = 7 - (move.from[1] - '1');
    int fromCol = move.from[0] - 'a';
    int toRow = 7 - (move.to[1] - '1');
    int toCol = move.to[0] - 'a';
    return makeAIMove(state, castleStatus, (fromRow*8) + fromCol, (toRow*8) + toCol);
}

unsigned int Chess::makeAIMove(char *state, int castleStatus, int fromIndex, int toIndex) {
    unsigned char fromPiece = state[fromIndex];
    state[fromIndex] = '0';
    unsigned char toPiece = state[toIndex];
    state[toIndex] = fromPiece;
    return ((fromIndex<<24) | (toIndex<<16) | fromPiece << 8 | toPiece);
}

void Chess::undoAIMove(char *state, unsigned int move) {
    unsigned char fromPiece = move >> 8;
    unsigned char toPiece = move & 0xFF;
    int fromIndex = (move >> 24) & 0xFF;
    int toIndex = (move >> 16) & 0xFF;
    state[fromIndex] = fromPiece;
    state[toIndex] = toPiece;
}

int Chess::minimaxAlphaBetaSorted(char* state, int depth, bool isMaximizingPlayer, int& castleStatus, int alpha, int beta) {
#if ZOBRISTHASH
    uint64_t zobristHash = _zobrist.ZobristHash(state, 64);
    int hashPos = zobristHash & (zobristSize-1);
    if (_zobristHashes[hashPos].age > 0 && _zobristHashes[hashPos].hash == zobristHash) {
        if (_zobristHashes[hashPos].depth >= depth && _zobristHashes[hashPos].alpha >= alpha) {
            _zobristHits++;
            return _zobristHashes[hashPos].score;
        }
    }
#endif
    int score = evaluateBoard(state);

    if (depth == DEPTH_LIMIT) {
        return score;
    }

    _depthSearches++;

    std::vector<std::pair<int, std::pair<int, int>>> moves; // Store score and move

    // Generate moves
    std::vector<Move> possibleMoves = generateMoves(state, castleStatus, gameStatus, _lastMove, (isMaximizingPlayer ? 'B' : 'W'));

    for (auto move : possibleMoves) {
        unsigned int aimove = makeAIMove(state, castleStatus, move);
        int moveScore = evaluateBoard(state); // Evaluate the move with a basic score function
        int fromIndex = (aimove >> 24) & 0xFF;
        int toIndex = (aimove >> 16) & 0xFF;
        moves.push_back({moveScore, {fromIndex, toIndex}});

        // Undo move
        undoAIMove(state, aimove);
    }

    // Sort moves: If isMaximizingPlayer, sort descending. Otherwise, sort ascending
    if (isMaximizingPlayer) {
        std::sort(moves.rbegin(), moves.rend());
    } else {
        std::sort(moves.begin(), moves.end());
    }

    if (isMaximizingPlayer) {
        int bestVal = -1000000;

        for (auto& move : moves) {
            auto [fromIndex, toIndex] = move.second;
            int aimove = makeAIMove(state, castleStatus, fromIndex, toIndex);
#if ZOBRISTHASH
            uint64_t maxHash = _zobrist.ZobristHash(state, 64);
            int hashPos = maxHash & (zobristSize-1);

            if (_zobristHashes[hashPos].age > 0 && _zobristHashes[hashPos].hash == maxHash) {
                _zobristHits++;
                bestVal = std::max(bestVal, _zobristHashes[hashPos].score);
            } else {
                _zobristMisses++;
                bestVal = std::max(bestVal, minimaxAlphaBetaSorted(state, depth + 1, !isMaximizingPlayer, castleStatus, alpha, beta));
                Transposition transposition = {bestVal, depth, alpha, beta, 1, maxHash};
                _zobristHashes[hashPos] = transposition;
            }
#else
            bestVal = std::max(bestVal, minimaxAlphaBetaSorted(state, depth + 1, !isMaximizingPlayer, castleStatus, alpha, beta));
#endif
            undoAIMove(state, aimove);
            alpha = std::max(alpha, bestVal);

            if (beta <= alpha) {
                return bestVal;
            }
        }

        return bestVal;
    } else {
        int bestVal = 1000000;

        for (auto& move : moves) {
            auto [fromIndex, toIndex] = move.second;
            int aimove = makeAIMove(state, castleStatus, fromIndex, toIndex);
            bestVal = std::min(bestVal, minimaxAlphaBetaSorted(state, depth + 1, !isMaximizingPlayer, castleStatus, alpha, beta));
            undoAIMove(state, aimove);
            beta = std::min(beta, bestVal);

            if (beta <= alpha) {
                return bestVal;
            }
        }

        return bestVal;
    }
}