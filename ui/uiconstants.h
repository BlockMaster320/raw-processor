#include <QString>

const QString baseButtonStyle = R"(
    QPushButton {
        border: none;
        border-radius: 10px;
        padding: 8px 14px;
        background-color: #3b3b3b;
        color: #dfe4ea;
        font-weight: 600;
    }

    QPushButton:hover {
        background-color: #454545;
    }

    QPushButton:pressed {
        background-color: #2f2f2f;
    }

    QPushButton:disabled {
        background-color: #2f2f2f;
        color: #7f8c8d;
    }
)";

const QString switchStyle = R"(
    QWidget {
        background-color: #3b3b3b;
        border-radius: 12px;
    }

    QPushButton {
        border: none;
        border-radius: 10px;
        padding: 6px 14px;
        background: transparent;
        color: #dfe4ea;
        font-weight: 600;
    }

    QPushButton:checked {
        background-color: #f1f2f6;
        color: #3b3b3b;
    }
)";