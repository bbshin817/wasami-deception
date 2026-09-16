"""This project's editor tooling: toolsets exposed through the ToolsetRegistry (and so the MCP server), and the asset
pipeline they run (wasami_tools.pipeline)."""
from toolset_registry.registration import Registration

from wasami_tools.toolsets import dd, dev, stage

_registration = Registration([stage.WasamiStageTools, dd.WasamiDDTools, dev.WasamiDevTools])


def register() -> bool:
    return _registration.register()


def unregister() -> None:
    _registration.unregister()
