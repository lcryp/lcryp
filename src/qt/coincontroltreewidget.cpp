#include <qt/coincontroltreewidget.h>
#include <qt/coincontroldialog.h>
CoinControlTreeWidget::CoinControlTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{
}
void CoinControlTreeWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space)
    {
        event->ignore();
        if (this->currentItem()) {
            int COLUMN_CHECKBOX = 0;
            this->currentItem()->setCheckState(COLUMN_CHECKBOX, ((this->currentItem()->checkState(COLUMN_CHECKBOX) == Qt::Checked) ? Qt::Unchecked : Qt::Checked));
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        event->ignore();
        CoinControlDialog *coinControlDialog = static_cast<CoinControlDialog*>(this->parentWidget());
        coinControlDialog->done(QDialog::Accepted);
    }
    else
    {
        this->QTreeWidget::keyPressEvent(event);
    }
}
