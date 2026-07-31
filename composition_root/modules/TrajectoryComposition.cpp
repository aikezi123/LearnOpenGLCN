#include "TrajectoryComposition.h"

#include <TrajectoryExportView.h>

namespace learnopengl::composition {

QWidget* TrajectoryComposition::createPage(QWidget* parent)
{
    return new ui::TrajectoryExportView(parent);
}

} // namespace learnopengl::composition
