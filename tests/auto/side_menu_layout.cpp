#include <QtTest/QtTest>

#include <applications/qt/common/side_menu.h>

#include <QApplication>
#include <QQuickItem>

// The side menu's shape: the program's own 3D options at the top (Enable 3D, the render resolution per eye,
// Display settings), everything that gives the picture another shape under Advanced, the path to the desktop
// among them, so it cannot be read as an output format.
class SideMenuLayout : public QObject
{
    Q_OBJECT

    static bool descends(const QObject* object, const QObject* ancestor)
    {
        for (const QObject* p = object; p; p = p->parent())
            if (p == ancestor)
                return true;
        return false;
    }

private Q_SLOTS:
    void pathChoiceIsUnderAdvanced()
    {
        all::qt::SideMenu menu(nullptr);
        QQuickItem* root = menu.rootItem();
        QVERIFY(root);
        QObject* stereoSection = root->findChild<QObject*>(QStringLiteral("stereoSection"));
        QObject* advancedSection = root->findChild<QObject*>(QStringLiteral("advancedSection"));
        QObject* pathCombo = root->findChild<QObject*>(QStringLiteral("stereoPathCombo"));
        QVERIFY(stereoSection);
        QVERIFY(advancedSection);
        QVERIFY(pathCombo);
        QVERIFY2(descends(pathCombo, advancedSection), "the path choice belongs under Advanced");
        QVERIFY2(!descends(pathCombo, stereoSection), "the top Stereo 3D section holds no path choice");

        // The top section keeps only Enable 3D, the render resolution per eye and Display settings
        int combos = 0;
        for (QObject* child : stereoSection->findChildren<QObject*>())
            if (QByteArray(child->metaObject()->className()).startsWith("QQuickComboBox"))
                ++combos;
        QCOMPARE(combos, 0);
    }

    // Every label of the top section wraps: a longer language (Brazilian Portuguese's "Resolução de renderização
    // por olho: 1544 x 1080") is not cut at the menu's width
    void stereoSectionLabelsWrap()
    {
        all::qt::SideMenu menu(nullptr);
        QQuickItem* root = menu.rootItem();
        QVERIFY(root);
        QObject* stereoSection = root->findChild<QObject*>(QStringLiteral("stereoSection"));
        QVERIFY(stereoSection);
        int labels = 0;
        // The section's own labels (a control's inner label is the control's); a Controls Label is a QML type of
        // the style ("Label_QMLTYPE_..."), a QQuickLabel underneath
        for (QObject* child : stereoSection->children()) {
            const QByteArray className = child->metaObject()->className();
            if (!className.startsWith("Label_") && !className.startsWith("QQuickLabel"))
                continue;
            ++labels;
            const QString text = child->property("text").toString();
            QVERIFY2(child->property("wrapMode").toInt() != 0, qPrintable(QStringLiteral("label does not wrap: ") + text));
        }
        QVERIFY2(labels >= 2, "the section's labels were found");
    }
};

QTEST_MAIN(SideMenuLayout)
#include "side_menu_layout.moc"
