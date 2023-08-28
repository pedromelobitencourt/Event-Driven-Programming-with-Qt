#ifndef CELL_H
#define CELL_H

#include <QPushButton>

class Player;

class Cell : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(Player* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(bool inverted READ isInverted WRITE setInverted NOTIFY invertedChanged)

public:
    explicit Cell(QWidget *parent = nullptr);
    virtual ~Cell();

    bool isInverted() const { return m_inverted; }
    void setInverted(bool inverted);
    
    bool isFixed() const { return m_fixed; }
    void setFixed(bool fixed);

    Player* player() const { return m_player; }
    void setPlayer(Player* player);

public slots:
    void reset();

signals:
    void playerChanged(Player* player);
    void invertedChanged(bool inverted);
    void fixedChanged(bool fixed);

private:
    Player* m_player;
    bool m_inverted;
    bool m_fixed;

private slots:
    void updateCell();

};

#endif // CELL_H
