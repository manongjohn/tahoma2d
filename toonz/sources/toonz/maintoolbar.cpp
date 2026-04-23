#include "maintoolbar.h"

#include "commandbar.h"
#include "commandbarpopup.h"

#include <QMenu>
#include <QContextMenuEvent>

MainToolbar::MainToolbar(QWidget *parent)
    : CommandBar(parent, Qt::WindowFlags(), false, CommandBarType::Main) {
}

MainToolbar::~MainToolbar() {}

void MainToolbar::contextMenuEvent(QContextMenuEvent *event) {
  QMenu *menu                  = new QMenu(this);
  QAction *customizeCommandBar = menu->addAction(tr("Customize Main Toolbar"));
  connect(customizeCommandBar, SIGNAL(triggered()),
          SLOT(doCustomizeCommandBar()));
  menu->exec(event->globalPos());
}

void MainToolbar::doCustomizeCommandBar() {
  CommandBarPopup *cbPopup = new CommandBarPopup("", CommandBarType::Main);

  if (cbPopup->exec()) {
    fillToolbar(this, CommandBarType::Main);
  }
  delete cbPopup;
}
