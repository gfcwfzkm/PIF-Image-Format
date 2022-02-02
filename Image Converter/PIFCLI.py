import io
import typer				# pip install typer
import PIL.Image			# pip install pillow
from enum import Enum		# requires python 3.10 or higher


def main(name: str):
	typer.echo(f"Hello {name}")


if __name__ == "__main__":
	typer.run(main)