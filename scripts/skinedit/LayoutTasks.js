//include_file ("translation.js")

function LayoutMode () {}
LayoutMode.kHorizontal = 0;
LayoutMode.kVertical = 1;

function SimpleLayoutTask (layoutMode)
{
	this.interfaces = [Host.Interfaces.IEditTask];
	this.layoutMode = layoutMode;

	// IEditTask
	this.prepareEdit = function (context)
	{
		return Host.Results.kResultOk;
	}
	
	this.performEdit = function (context)
	{
		let iterator = context.iterator;
		let functions = context.functions;
		let focusItem = context.focusItem;
		if(!iterator || !functions || !focusItem)
			return Host.Results.kResultFailed;
		
		let c = 0;
		if(this.layoutMode == LayoutMode.kHorizontal)
			c = focusItem.getSize ().top;
		else
			c = focusItem.getSize ().left;
						
		while(!iterator.done ())
		{
			let viewItem = iterator.next ();
			//Host.Console.writeLine (viewItem.getDisplayName ());

			let size = viewItem.getSize ().clone ();
			if(this.layoutMode == LayoutMode.kHorizontal)
			{
				let height = size.bottom - size.top;
				size.top = c;
				size.bottom = c + height;
			}
			else
			{
				let width = size.right - size.left;
				size.left = c;
				size.right = c + width;
			}
						
			functions.resizeView (viewItem, size);
		}
		return Host.Results.kResultOk;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

function createLayoutHTask ()
{
	return new SimpleLayoutTask (LayoutMode.kHorizontal);
}

function createLayoutVTask ()
{
	return new SimpleLayoutTask (LayoutMode.kVertical);
}
