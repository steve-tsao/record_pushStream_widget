/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 6.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QGridLayout *gridLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *verticalLayout;
    QRadioButton *radioButton_3;
    QRadioButton *radioButton_2;
    QLCDNumber *lcdNumber_3;
    QLCDNumber *lcdNumber_2;
    QLCDNumber *lcdNumber;
    QPushButton *pushButton;
    QTabWidget *tabWidget;
    QWidget *tab;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QComboBox *comboBox;
    QLabel *label_2;
    QSpinBox *spinBox;
    QLabel *label_3;
    QSlider *horizontalSlider_2;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *pushButton_2;
    QLineEdit *lineEdit;
    QPushButton *pushButton_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label_4;
    QLineEdit *lineEdit_2;
    QWidget *tab1;
    QRadioButton *radioButton;
    QSlider *horizontalSlider;
    QLabel *label_5;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName("Widget");
        Widget->resize(360, 203);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Widget->sizePolicy().hasHeightForWidth());
        Widget->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(Widget);
        gridLayout_2->setObjectName("gridLayout_2");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        radioButton_3 = new QRadioButton(Widget);
        radioButton_3->setObjectName("radioButton_3");

        verticalLayout->addWidget(radioButton_3);

        radioButton_2 = new QRadioButton(Widget);
        radioButton_2->setObjectName("radioButton_2");

        verticalLayout->addWidget(radioButton_2);


        horizontalLayout_4->addLayout(verticalLayout);

        lcdNumber_3 = new QLCDNumber(Widget);
        lcdNumber_3->setObjectName("lcdNumber_3");
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lcdNumber_3->sizePolicy().hasHeightForWidth());
        lcdNumber_3->setSizePolicy(sizePolicy1);
        lcdNumber_3->setFrameShape(QFrame::StyledPanel);
        lcdNumber_3->setFrameShadow(QFrame::Raised);
        lcdNumber_3->setSmallDecimalPoint(false);
        lcdNumber_3->setMode(QLCDNumber::Dec);
        lcdNumber_3->setSegmentStyle(QLCDNumber::Flat);

        horizontalLayout_4->addWidget(lcdNumber_3);

        lcdNumber_2 = new QLCDNumber(Widget);
        lcdNumber_2->setObjectName("lcdNumber_2");
        sizePolicy1.setHeightForWidth(lcdNumber_2->sizePolicy().hasHeightForWidth());
        lcdNumber_2->setSizePolicy(sizePolicy1);
        lcdNumber_2->setFrameShape(QFrame::StyledPanel);
        lcdNumber_2->setSegmentStyle(QLCDNumber::Flat);

        horizontalLayout_4->addWidget(lcdNumber_2);

        lcdNumber = new QLCDNumber(Widget);
        lcdNumber->setObjectName("lcdNumber");
        sizePolicy1.setHeightForWidth(lcdNumber->sizePolicy().hasHeightForWidth());
        lcdNumber->setSizePolicy(sizePolicy1);
        lcdNumber->setFrameShape(QFrame::StyledPanel);
        lcdNumber->setSegmentStyle(QLCDNumber::Flat);

        horizontalLayout_4->addWidget(lcdNumber);

        pushButton = new QPushButton(Widget);
        pushButton->setObjectName("pushButton");
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy2);

        horizontalLayout_4->addWidget(pushButton);


        gridLayout_2->addLayout(horizontalLayout_4, 1, 0, 1, 1);

        tabWidget = new QTabWidget(Widget);
        tabWidget->setObjectName("tabWidget");
        tab = new QWidget();
        tab->setObjectName("tab");
        gridLayout = new QGridLayout(tab);
        gridLayout->setObjectName("gridLayout");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label = new QLabel(tab);
        label->setObjectName("label");

        horizontalLayout_3->addWidget(label);

        comboBox = new QComboBox(tab);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
        comboBox->setSizePolicy(sizePolicy3);
        comboBox->setMinimumSize(QSize(61, 21));
        comboBox->setMaximumSize(QSize(61, 21));

        horizontalLayout_3->addWidget(comboBox);

        label_2 = new QLabel(tab);
        label_2->setObjectName("label_2");

        horizontalLayout_3->addWidget(label_2);

        spinBox = new QSpinBox(tab);
        spinBox->setObjectName("spinBox");
        spinBox->setEnabled(true);
        spinBox->setFocusPolicy(Qt::ClickFocus);
        spinBox->setAutoFillBackground(false);
        spinBox->setReadOnly(false);
        spinBox->setKeyboardTracking(false);

        horizontalLayout_3->addWidget(spinBox);

        label_3 = new QLabel(tab);
        label_3->setObjectName("label_3");

        horizontalLayout_3->addWidget(label_3);

        horizontalSlider_2 = new QSlider(tab);
        horizontalSlider_2->setObjectName("horizontalSlider_2");
        horizontalSlider_2->setMinimum(1);
        horizontalSlider_2->setMaximum(20000);
        horizontalSlider_2->setTracking(true);
        horizontalSlider_2->setOrientation(Qt::Horizontal);

        horizontalLayout_3->addWidget(horizontalSlider_2);


        gridLayout->addLayout(horizontalLayout_3, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        pushButton_2 = new QPushButton(tab);
        pushButton_2->setObjectName("pushButton_2");
        QSizePolicy sizePolicy4(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(pushButton_2->sizePolicy().hasHeightForWidth());
        pushButton_2->setSizePolicy(sizePolicy4);
        pushButton_2->setMaximumSize(QSize(130, 16777215));

        horizontalLayout_2->addWidget(pushButton_2);

        lineEdit = new QLineEdit(tab);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setEnabled(true);
        lineEdit->setFocusPolicy(Qt::NoFocus);
        lineEdit->setFrame(true);
        lineEdit->setDragEnabled(false);
        lineEdit->setReadOnly(false);

        horizontalLayout_2->addWidget(lineEdit);

        pushButton_3 = new QPushButton(tab);
        pushButton_3->setObjectName("pushButton_3");
        QSizePolicy sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Maximum);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(pushButton_3->sizePolicy().hasHeightForWidth());
        pushButton_3->setSizePolicy(sizePolicy5);
        pushButton_3->setMaximumSize(QSize(32, 16777215));

        horizontalLayout_2->addWidget(pushButton_3);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label_4 = new QLabel(tab);
        label_4->setObjectName("label_4");

        horizontalLayout->addWidget(label_4);

        lineEdit_2 = new QLineEdit(tab);
        lineEdit_2->setObjectName("lineEdit_2");

        horizontalLayout->addWidget(lineEdit_2);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);

        tabWidget->addTab(tab, QString());
        tab1 = new QWidget();
        tab1->setObjectName("tab1");
        radioButton = new QRadioButton(tab1);
        radioButton->setObjectName("radioButton");
        radioButton->setGeometry(QRect(30, 20, 59, 19));
        horizontalSlider = new QSlider(tab1);
        horizontalSlider->setObjectName("horizontalSlider");
        horizontalSlider->setGeometry(QRect(180, 20, 71, 22));
        horizontalSlider->setOrientation(Qt::Horizontal);
        label_5 = new QLabel(tab1);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(110, 20, 53, 15));
        tabWidget->addTab(tab1, QString());

        gridLayout_2->addWidget(tabWidget, 0, 0, 1, 1);


        retranslateUi(Widget);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "\345\275\225\345\261\217", nullptr));
        radioButton_3->setText(QCoreApplication::translate("Widget", "\346\216\250\346\265\201", nullptr));
        radioButton_2->setText(QCoreApplication::translate("Widget", "\345\275\225\345\210\266", nullptr));
        pushButton->setText(QCoreApplication::translate("Widget", "\345\274\200\345\247\213", nullptr));
        label->setText(QCoreApplication::translate("Widget", "\346\240\274\345\274\217", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("Widget", "mp4", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("Widget", "avi", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("Widget", "flv", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("Widget", "mkv", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("Widget", "mov", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("Widget", "wmv", nullptr));
        comboBox->setItemText(6, QCoreApplication::translate("Widget", "rmvb", nullptr));

        label_2->setText(QCoreApplication::translate("Widget", "\345\270\247\347\216\207", nullptr));
        spinBox->setSpecialValueText(QString());
        label_3->setText(QCoreApplication::translate("Widget", "\347\240\201\347\216\207", nullptr));
        pushButton_2->setText(QCoreApplication::translate("Widget", "\344\277\235\345\255\230\350\267\257\345\276\204", nullptr));
        pushButton_3->setText(QCoreApplication::translate("Widget", "\346\265\217\350\247\210", nullptr));
        label_4->setText(QCoreApplication::translate("Widget", "\346\216\250\346\265\201\345\234\260\345\235\200\357\274\232", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("Widget", "\350\256\276\347\275\256", nullptr));
        radioButton->setText(QCoreApplication::translate("Widget", "\350\247\243\345\244\215\347\224\250", nullptr));
        label_5->setText(QCoreApplication::translate("Widget", "\347\240\201\347\216\207\345\256\275\345\256\271\357\274\232", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab1), QCoreApplication::translate("Widget", "\351\253\230\347\272\247", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
