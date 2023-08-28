#include "Squadro.h"
#include "ui_Squadro.h"
#include "Cell.h"
#include "Player.h"

#include <QDebug>
#include <QMessageBox>
#include <QSignalMapper>

#include <string>

Squadro::Squadro(QWidget *parent)
    : QMainWindow(parent),
        ui(new Ui::Squadro),
        m_player(nullptr) {

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            QString cellName = QString("cell%1%2").arg(row).arg(col);
            Cell* cell = this->findChild<Cell*>(cellName);
            m_board[row][col] = cell;

            if (cell != nullptr) {
                int id = row * 7 + col;
                map->setMapping(cell, id);
                QObject::connect(cell, SIGNAL(clicked(bool)), map, SLOT(map()));
            }
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends, switch the player.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(switchPlayer()));

    // Connect the red player counts.
    Player* red = Player::player(Player::Red);
    QObject::connect(red, SIGNAL(countChanged(int)), this, SLOT(updateStatusBar()));

    // Connect the blue player counts.
    Player* blue = Player::player(Player::Blue);
    QObject::connect(blue, SIGNAL(countChanged(int)), this, SLOT(updateStatusBar()));

    // Reset.
    this->reset();

    // Adjust window.
    this->adjustSize();
    this->setFixedSize(this->size());
}

Squadro::~Squadro() {
    delete ui;
}

void Squadro::play(int id) {
    int row = id / 7;
    int col = id % 7;
    Cell* cell = m_board[row][col];
    Q_ASSERT(cell != nullptr);

    if ((cell->player() == m_player) and (!cell->isFixed()) and (!acabouPartida)) {
        std::string s = std::to_string(id);
        s += "\nrow: " + std::to_string(row) + "\ncol: " + std::to_string(col);
        char arrS[s.length() + 1];
        strcpy(arrS, s.c_str());

        QMessageBox::information(this, tr("About"), tr(arrS));

        Squadro::verificacaoInimigos(id);

        emit turnEnded();
    }
}

void Squadro::verificacaoInimigos(int id) {
    int row = id / 7;
    int col = id % 7;

    if(m_player->name().localeAwareCompare("Jogador Azul") == 0) { // Precisamos da nova linha
        /*std::string s = "Blue\n";
        char arrS[s.length() + 1];
        strcpy(arrS, s.c_str());

        QMessageBox::information(this, tr("About"), tr(arrS)); */

        Player* ally = Player::player(Player::Blue); // Azul
        Player* enemy = Player::player(Player::Red); // Vermelho

        int numeroJogador = col; // Para o jogador azul, podemos saber qual peça está sendo jogada no momento com 'id % 7' == col
        int powerNotInverted = 0;
        int powerInverted = 0;
        int power = 0;
        int newRow = 0;
        Cell *currentCell = m_board[row][col];

        switch(numeroJogador) {
            case 1:
            case 5:
                powerNotInverted = 1;
                powerInverted = 3;
                break;
            case 2:
            case 4:
                powerNotInverted = 3;
                powerInverted = 1;
                break;
            case 3:
                powerNotInverted = powerInverted = 2;
                break;
            default:
                QMessageBox::information(this, tr("Error"),
                tr("Unexpected error : line 284, Squadro"));
        }

        int posicaoUltimoInimigoRelacaoPower = 0;
        Cell* c;

        // A peça do jogador atual esta invertida. Dessa maneira, os jogadores se deslocam para frente AUMENTANDO a linha (Row)
        if(currentCell->isInverted()) {
            power = powerInverted;

            for(int i = 1; i <= power; i++) {
                if(row + i > 5) // Não tem inimigos na linha 6 e linhas maiores de 6 não existem
                    break;

                c = m_board[row + i][col];

                if(c->player() != nullptr) { // Há inimigo no caminho
                    posicaoUltimoInimigoRelacaoPower = i;

                    bool estaInvertido = c->isInverted(); // Armazena a informação se a célula está invertida ou não, pois ela será deletada
                    c->reset(); // Mata-se o inimigo

                    int respawnCol = 0;

                    // O inimigo (red) está voltando à origem, ou seja, seu respawn é a partir da coluna 6
                    if(estaInvertido) {
                        respawnCol = 6;
                        c = m_board[row + i][respawnCol];
                        c->setInverted(true);
                    }
                    // O inimigo (red) está ainda na IDA, ou seja, seu respawn é na coluna 0
                    else {
                        respawnCol = 0;
                        c = m_board[row + i][respawnCol];
                        c->setInverted(false);
                    }
                    c->setPlayer(enemy);
                }
            }


            if(posicaoUltimoInimigoRelacaoPower == 0) // Nao foi encontrado inimigos no caminho
                newRow = row + power;
            else // Encontrou-se inimigos no meio do caminho
                newRow = row + posicaoUltimoInimigoRelacaoPower + 1;

            if(newRow < 6) { // Evitar erro
                // Se tiver um inimigo adiante
                c = m_board[newRow][col];

                while((c->player() != nullptr) and (newRow < 6)) { // Encontrou mais inimigos (red)
                    bool estaInvertido = c->isInverted();
                    c->reset(); // Mata o inimigo (red)
                    int respawnCol = 0;

                    // Quando o inimigo (red) está invertido, a sua coluna de respawn é 6
                    if(estaInvertido) {
                        respawnCol = 6;
                        c = m_board[row][respawnCol];
                    }
                    // Quando o inimigo (red) não está invertido, a sua coluna de respawn é 0
                    else {
                        respawnCol = 0;
                        c = m_board[row][respawnCol];
                    }
                    c->setPlayer(enemy);
                    c->setInverted(estaInvertido);
                    newRow += 1; // Não passa de 6
                    c = m_board[newRow][col];
                }
            }

            
            if(newRow > 6) {
                newRow = 6;
            }

            currentCell->reset();
            currentCell = m_board[newRow][col];
            currentCell->setPlayer(ally);
            currentCell->setInverted(true);

            if(newRow == 6) {
                currentCell->setFixed(true); // Não pode mais se mover
                m_player->incrementCount(); // Chegou no destino final

                if(m_player->count() >= 4)
                    Squadro::vitoria();
            }

            // TODO: FIXAR JOGADOR
        }
        // Nao esta invertido. Dessa maneira, o jogador se movimenta para frente DIMINUINDO o número da linha (Row)
        else{
            power = powerNotInverted;

            for(int i = 1; i <= power; i++) {
                if(row - i < 1) // Nao tem inimigo na linha 0 e linha menores que zero nao existem
                    break;

                c = m_board[row - i][col];

                if(c->player() != nullptr) { // Há inimigo no meio do caminho
                    posicaoUltimoInimigoRelacaoPower = i;

                    bool estaInvertido = c->isInverted(); // Armazena-se a informação se a célula está invertida, pois o inimigo será removido
                    c->reset(); // Mata-se o inimigo

                    int respawnCol = 0;

                    // O inimigo (red) está invertido, logo seu respawn é a coluna 6
                    if(estaInvertido) {
                        respawnCol = 6;
                        c = m_board[row - i][respawnCol];
                        c->setInverted(true);
                    }

                    // O inimigo (red) não está invertido, logo seu respawn é a coluna 0
                    else{
                        respawnCol = 0;
                        c = m_board[row - i][respawnCol];
                        c->setInverted(false);
                    }
                    c->setPlayer(enemy);
                }
            }

            if(posicaoUltimoInimigoRelacaoPower == 0) // Não foram encontrados inimigos
                newRow = row - power;
            else
                newRow = row - (posicaoUltimoInimigoRelacaoPower + 1);

            if(newRow > 0) { // Evitar erro
                // Se ha inimigo adiante
                c = m_board[newRow][col];

                while((c->player() != nullptr) and (newRow > 0)) {
                    bool estaInvertido = c->isInverted();
                    c->reset();
                    int respawnCol = 0;

                    // Quando o inimigo (red) está invertido, sua coluna de respawn é 6
                    if(estaInvertido) {
                        respawnCol = 6;
                        c = m_board[newRow][respawnCol];
                    }
                    // Quando o inimigo (red) não está invertido, sua coluna de respawn é 0
                    else {
                        respawnCol = 0;
                        c = m_board[newRow][respawnCol];
                    }
                    c->setInverted(estaInvertido);
                    c->setPlayer(enemy);
                    newRow -= 1;
                    c = m_board[newRow][col];
                }
            }

            if(newRow < 0)
                newRow = 0;
            
            currentCell->reset();
            currentCell = m_board[newRow][col];
            currentCell->setPlayer(ally);

            if(newRow == 0)
                currentCell->setInverted(true);
            else
                currentCell->setInverted(false);
        }

    }
    else if(m_player->name().localeAwareCompare("Jogador Vermelho") == 0) { // Precisamos da nova coluna
        /*std::string s = "Red\n";
        char arrS[s.length() + 1];
        strcpy(arrS, s.c_str());

        QMessageBox::information(this, tr("About"), tr(arrS)); */

        Player* ally = Player::player(Player::Red);
        Player* enemy = Player::player(Player::Blue);

        int numeroJogador = 0;
        int powerNotInverted = 0;
        int powerInverted = 0;
        int power = 0;
        int newCol = 0;
        Cell* currentCell = m_board[row][col];

        if((id > 6) && (id < 14))
            numeroJogador = 1;
        else if((id > 13) && (id < 21))
            numeroJogador = 2;
        else if((id > 20) && (id < 28))
            numeroJogador = 3;
        else if((id > 27) && (id < 35))
            numeroJogador = 4;
        else if((id > 34) && (id < 42))
            numeroJogador = 5;

        switch(numeroJogador) {
            case 1:
            case 5:
                powerNotInverted = 1;
                powerInverted = 3;
                break;
            case 2:
            case 4:
                powerNotInverted = 3;
                powerInverted = 1;
                break;
            case 3:
                powerNotInverted = powerInverted = 2;
                break;
            default:
                QMessageBox::information(this, tr("Error"),
                tr("Unexpected error : line 443, Squadro"));
        }

        int posicaoUltimoInimigoRelacaoPower = 0;
        Cell* c;

        // A peça do jogador atual está invertida. Dessa forma, os jogadores se deslocam DIMINUINDO a coluna (col)
        if(currentCell->isInverted()) {
            power = powerInverted;

            for(int i = 1; i <= power; i++) {
                if(col - i < 1) // Não há inimigos na linha 0 e nas linhas menores que 0
                    break;

                c = m_board[row][col - i];

                if(c->player() != nullptr) { // Há inimigo no caminho
                    posicaoUltimoInimigoRelacaoPower = i;

                    bool estaInvertido = c->isInverted();
                    c->reset(); // Kills the enemy

                    int respawnRow = 0;

                    if(estaInvertido) {
                        respawnRow = 0; // Se o inimigo (blue) está invertido, ele deve voltar à linha 0
                        c = m_board[respawnRow][col - i];
                        c->setInverted(true); 
                    }
                    else {
                        respawnRow = 6; // Se o inimigo (blue) não está invertido, ele deve voltar à linha 6
                        c = m_board[respawnRow][col - i];
                        c->setInverted(false);
                    }
                    c->setPlayer(enemy);
                }
            }

            if(posicaoUltimoInimigoRelacaoPower == 0) // Não tinha inimigos no meio do caminho
                newCol = col - power;
            else
                newCol = col - (posicaoUltimoInimigoRelacaoPower + 1);

            if(newCol > 0) {  // Evitar erro
                // Se tiver um inimigo adiante
                c = m_board[row][newCol];

                while((c->player() != nullptr) and (newCol > 0)) {
                    bool estaInvertido = c->isInverted();
                    c->reset();
                    int respawnRow = 0;

                    // Quando o inimigo (blue) está invertido, a sua linha de respawn é 0
                    if(estaInvertido) {
                        respawnRow = 0;
                        c = m_board[respawnRow][newCol];
                    }
                    // Quando o inimigo (blue) não está invertido, a sua linha de respawn é 6
                    else {
                        respawnRow = 6;
                        c = m_board[respawnRow][newCol];
                    }
                    c->setPlayer(enemy);
                    c->setInverted(estaInvertido);
                    newCol -= 1; // Não fica menos de 0
                    c = m_board[row][newCol];
                }
            }
            
            if(newCol < 0) {
                newCol = 0;
                m_player->incrementCount();
            }

            currentCell->reset();
            currentCell = m_board[row][newCol];
            currentCell->setPlayer(ally);
            currentCell->setInverted(true);

            if(newCol == 0) {
                currentCell->setFixed(true);
                m_player->incrementCount();

                if(m_player->count() >= 4)
                    Squadro::vitoria();
            }
        }
        // Não está invertido o jogador vermelho. Assim, ele se desloca AUMENTANDO o número da coluna (col)
        else {
            power = powerNotInverted;

            for(int i = 1; i <= power; i++) {
                if(col + i > 5) // Não há inimigos além da coluna 5
                    break;
                
                c = m_board[row][col + i];

                if(c->player() != nullptr) { // Encontrou-se inimigos no caminho
                    posicaoUltimoInimigoRelacaoPower = i;

                    bool estaInvertido = c->isInverted();
                    c->reset();

                    int respawnRow = 0;

                    // Se o inimigo (blue) está invertido, a linha de respawn dele é a 0
                    if(estaInvertido) { 
                        respawnRow = 0;
                        c = m_board[respawnRow][col + i];
                        c->setInverted(true);
                    }
                    // Se o inimigo (blue) não está invertido, a linha de respawn é 6
                    else {
                        respawnRow = 6;
                        c = m_board[respawnRow][col + i];
                        c->setInverted(false);
                    }
                    c->setPlayer(enemy);
                }
            }

            if(posicaoUltimoInimigoRelacaoPower == 0)
                newCol = col + power;
            else
                newCol = col + (posicaoUltimoInimigoRelacaoPower + 1);

            if(newCol < 6) { // Evitar erro
                // Se tiver um inimigo adiante
                c = m_board[row][newCol];

                while((c->player() != nullptr) and (newCol < 6)) {
                    bool estaInvertido = c->isInverted();
                    c->reset();
                    int respawnRow = 0;

                    // Quando o inimigo (blue) está invertido, a sua linha de respawn é 0
                    if(estaInvertido) {
                        respawnRow = 0;
                        c = m_board[respawnRow][newCol];
                        c->setInverted(true);
                    }
                    // Quando o inimigo (blue) não está invertido, a sua linha de respawn é 6
                    else {
                        respawnRow = 6;
                        c = m_board[respawnRow][newCol];
                        c->setInverted(false);
                    }
                    c->setPlayer(enemy);
                    newCol += 1; // Não consegue ser maior que 6
                    c = m_board[row][newCol];
                }
            }
            
            if(newCol > 6)
                newCol = 6;
            
            currentCell->reset();
            currentCell = m_board[row][newCol];
            currentCell->setPlayer(ally);

            if(newCol == 6)
                currentCell->setInverted(true);
            else
                currentCell->setInverted(false);

        }

    }
    else {
        std::string s = "default\n";
        char arrS[s.length() + 1];
        strcpy(arrS, s.c_str());

        QMessageBox::information(this, tr("About"), tr(arrS));
    }

}

void Squadro::vitoria() {
    std::string s = "Parabéns, o ";
    if(m_player->name().localeAwareCompare("Jogador Azul") == 0)
        s += "Jogador Azul venceu.";
    else
        s += "Jogador Vermelho venceu.";
    
    char arrS[s.length() + 1];
    strcpy(arrS, s.c_str());

    acabouPartida = true;

    QMessageBox::information(this, tr("Vencedor"),
    tr(arrS));
}

void Squadro::switchPlayer() {
    // Switch the player.
    m_player = m_player->other();

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Squadro::reset() {
    // Reset the red player.
    Player* red = Player::player(Player::Red);
    red->reset();

    // Reset the blue player.
    Player* blue = Player::player(Player::Blue);
    blue->reset();

    // Reset board.
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 7; ++col) {
            Cell* cell = m_board[row][col];
            if (cell != nullptr)
                cell->reset();
        }
    }

    for (int i = 1; i < 6; i++) {
        m_board[i][0]->setPlayer(red);
        m_board[6][i]->setPlayer(blue);
    }

    // Set the starting player.
    m_player = red;

    acabouPartida = false;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Squadro::showAbout() {
    QMessageBox::information(this, tr("About"),
        tr("Squadro\n\nAndrei Rimsa Alvares - andrei@cefetmg.br"));
}

void Squadro::updateStatusBar() {
    ui->statusbar->showMessage(tr("Vez do %1 (%2 de 5)").arg(m_player->name()).arg(m_player->count()));
}
