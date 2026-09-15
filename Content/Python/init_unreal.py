"""Editor start-up: registers this project's toolsets (wasami_tools) with the ToolsetRegistry, which puts them on the
MCP server."""
import wasami_tools

wasami_tools.register()
