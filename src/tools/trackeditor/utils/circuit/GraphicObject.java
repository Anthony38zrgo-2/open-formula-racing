package utils.circuit;

import java.awt.geom.Point2D;

import gui.properties.GraphicObjectData;

public class GraphicObject implements Cloneable
{
	private ObjShapeObject	shape		= null;
	private String			comment		= null;

	// TODO add more overrides here
	private double 			orientation = Double.NaN;
	private double			height		= Double.NaN;
	private String			useMaterial	= null;

	public GraphicObject(String name, int rgb, Point2D.Double location)
	{
		shape = new ObjShapeObject(name, rgb, location);
	}

	public GraphicObject(String name, int rgb, Point2D.Double location, String comment, double orientation, double height, String useMaterial)
	{
		shape = new ObjShapeObject(name, rgb, location);
		this.comment = comment;
		this.orientation = orientation;
		this.height = height;
		this.useMaterial = useMaterial;
	}

	public GraphicObject(GraphicObjectData data)
	{
		shape = new ObjShapeObject(data.name, data.color, new Point2D.Double(data.trackX, data.trackY));
		comment = data.comment;
		orientation = data.orientation;
		height = data.height;
		useMaterial = data.useMaterial;
	}

	public GraphicObject(ObjShapeObject shape)
	{
		this.shape = shape;
	}

	public Object clone()
	{
		GraphicObject object = new GraphicObject((ObjShapeObject) this.shape.clone());
		object.comment = this.comment;
		object.orientation = this.orientation;
		object.height = this.height;
		object.useMaterial = this.useMaterial;
		return object;
	}

	public ObjShapeObject getShape()
	{
		return shape;
	}

	public void setShape(ObjShapeObject shape)
	{
		this.shape = shape;
	}

	public String getName()
	{
		return shape.getName();
	}

	public void setName(String name)
	{
		shape.setName(name);
	}

	public int getColor()
	{
		return shape.getRGB();
	}

	public void setColor(int rgb)
	{
		shape.setRGB(rgb);
	}

	public double getX()
	{
		return shape.getTrackLocation().x;
	}

	public void setX(double x)
	{
		shape.setTrackLocationX(x);
	}

	public double getY()
	{
		return shape.getTrackLocation().y;
	}

	public void setY(double y)
	{
		shape.setTrackLocationY(y);
	}

	public Point2D.Double getLocation()
	{
		return shape.getTrackLocation();
	}

	public void setLocation(Point2D.Double location)
	{
		shape.setLocation(location);
	}

	public String getComment()
	{
		return comment;
	}

	public void setComment(String comment)
	{
		this.comment = comment;
	}

	public double getOrientation()
	{
		return orientation;
	}

	public void setOrientation(double orientation)
	{
		this.orientation = orientation;
	}

	public double getHeight()
	{
		return height;
	}

	public void setHeight(double height)
	{
		this.height = height;
	}

	public String getUseMaterial()
	{
		return useMaterial;
	}

	public void setUseMaterial(String useMaterial)
	{
		this.useMaterial = useMaterial;
	}

	public void dump(String indent)
	{
		System.out.println(indent + "name         : " + getName());
		System.out.println(indent + "color        : " + getColor());
		System.out.println(indent + "x            : " + getX());
		System.out.println(indent + "y            : " + getY());
		System.out.println(indent + "orientation  : " + getOrientation());
		System.out.println(indent + "height       : " + getHeight());
		System.out.println(indent + "use material : " + getUseMaterial());
	}
}
