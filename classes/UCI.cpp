#include "UCI.h"
#include "../Application.h"

void UCIInterface::UCILog(std::string message)
{
    // append message to a log file
    std::ofstream logFile;
    logFile.open("UCI.log", std::ios_base::app);
    logFile << message << std::endl;
    logFile.close();
}

void UCIInterface::FENtoBoard(const std::string &fen)
{
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    // Clear the board
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            _instance->getHolderAt(i, j).setBit(nullptr);
        }
    }

    std::istringstream fenStream(fen);
    std::string boardPart;
    std::getline(fenStream, boardPart, ' ');

    int row = 7;
    int col = 0;
    for (char ch : boardPart)
    {
        if (ch == '/')
        {
            row--;
            col = 0;
        }
        else if (isdigit(ch))
        {
            col += ch - '0'; // Skip empty squares
        }
        else
        {
            // convert ch to a piece
            ChessPiece piece = Pawn;
            switch (toupper(ch))
            {
            case 'P':
                piece = Pawn;
                break;
            case 'N':
                piece = Knight;
                break;
            case 'B':
                piece = Bishop;
                break;
            case 'R':
                piece = Rook;
                break;
            case 'Q':
                piece = Queen;
                break;
            case 'K':
                piece = King;
                break;
            }
            Bit *bit = _instance->PieceForPlayer(isupper(ch) ? 0 : 1, piece);
            BitHolder &holder = _instance->getHolderAt(col, row);
            bit->setPosition(holder.getPosition());
            bit->setParent(&holder);
            bit->setGameTag(isupper(ch) ? piece : (piece + 128));
            bit->setSize(pieceSize, pieceSize);
            holder.setBit(bit);
            col++;
        }
    }
}
