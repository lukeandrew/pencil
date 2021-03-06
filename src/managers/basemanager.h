#ifndef BASEMANAGER_H
#define BASEMANAGER_H

#include <QObject>

class Editor;


class BaseManager : public QObject
{
    Q_OBJECT
public:
    explicit BaseManager(QObject *parent = 0);

    Editor* editor();
    void    setEditor( Editor* pEditor );
    
    virtual bool initialize() = 0;

private:
    Editor* m_pEditor;
};

#endif // BASEMANAGER_H
