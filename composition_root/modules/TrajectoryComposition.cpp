#include "TrajectoryComposition.h"

#include <TrajectoryExportView.h>

namespace engineeringlab::composition {

QWidget* TrajectoryComposition::createPage(QWidget* parent)
{
    return new ui::TrajectoryExportView(parent);
}

} // namespace engineeringlab::composition
