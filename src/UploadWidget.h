#ifndef UPLOADWIDGET_H
#define UPLOADWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QNetworkReply>

class UploadWidget : public QWidget {
    Q_OBJECT
public:
    explicit UploadWidget(QWidget *parent = nullptr);

private slots:
    void browseFile();
    void uploadFile();
private:
    QLineEdit* m_fileEdit;
    QLineEdit* m_topicEdit;
    QPushButton* m_browseBtn;
    QPushButton* m_uploadBtn;
    QLabel* m_statusLabel;
};

#endif // UPLOADWIDGET_H
