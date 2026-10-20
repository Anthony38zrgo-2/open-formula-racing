package gui.properties;

import utils.circuit.GraphicObject;
import utils.circuit.ObjShapeObject;

public class GraphicObjectData implements Cloneable
{
	public String	name;
	public String	comment;
	public Integer	color;
	public Double	trackX;
	public Double	trackY;
	public Double	orientation;
	public Double	height;
	public String	useMaterial;

	public GraphicObjectData(String name, String comment, Integer color, double trackX, double trackY, double orientation, double height, String useMaterial)
	{
		this.name = name;
		this.comment = comment;
		this.color = color;
		this.trackX = trackX;
		this.trackY = trackY;
		this.orientation = orientation;
		this.height = height;
		this.useMaterial = useMaterial;
	}

	public GraphicObjectData(GraphicObjectData data)
	{
		this.name = data.name;
		this.comment = data.comment;
		this.color = data.color;
		this.trackX = data.trackX;
		this.trackY = data.trackY;
		this.orientation = data.orientation;
		this.height = data.height;
		this.useMaterial = data.useMaterial;
	}

	public GraphicObjectData(GraphicObject object)
	{
		this.name = object.getName();
		this.comment = object.getComment();
		this.color = object.getColor();
		this.trackX = object.getLocation().x;
		this.trackY = object.getLocation().y;
		this.orientation = object.getOrientation();
		this.height = object.getHeight();
		this.useMaterial = object.getUseMaterial();
	}

	public GraphicObjectData(ObjShapeObject object)
	{
		this.name = object.getName();
		this.comment = object.getComment();
		this.color = object.getRGB();
		this.trackX = object.getTrackLocation().x;
		this.trackY = object.getTrackLocation().y;
		this.orientation = Double.NaN;
		this.height = Double.NaN;
		this.useMaterial = null;
	}

	public Object clone()
	{
		return new GraphicObjectData(this);
	}
}
