
/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include <osgUtil/IntersectionVisitor>

class IntersectorPrivate : public osgUtil::Intersector
{
  public:

    /** Convenience constructor for supporting picking in WINDOW, or PROJECTION
     * coordinates In WINDOW coordinates creates a start value of (x,y,0) and
     * end value of (x,y,1).  In PROJECTION coordinates (clip space cube)
     * creates a start value of (x,y,-1) and end value of (x,y,1).  In VIEW and
     * MODEL coordinates creates a start value of (x,y,0) and end value of
     * (x,y,1).*/
    IntersectorPrivate(
      CoordinateFrame cf,
      double x,
      double y,
      double thickness
    );

    struct Intersection
    {
      Intersection():
        ratio(-1.0),
        primitiveIndex(0) {}

      bool operator < (const Intersection& rhs) const { return ratio < rhs.ratio; }

      typedef std::vector<unsigned int>   IndexList;
      typedef std::vector<double>         RatioList;

      double                          ratio;
      osg::NodePath                   nodePath;
      osg::ref_ptr<osg::Drawable>     drawable;
      osg::ref_ptr<osg::RefMatrix>    matrix;
      osg::Vec3d                      localIntersectionPoint;
      osg::Vec3                       localIntersectionNormal;
      IndexList                       indexList;
      RatioList                       ratioList;
      unsigned int                    primitiveIndex;

      const osg::Vec3d& getLocalIntersectPoint() const
      {
        return localIntersectionPoint;
      }

      osg::Vec3d getWorldIntersectPoint() const
      {
        return
          matrix.valid() ?
            localIntersectionPoint * (*matrix) :
            localIntersectionPoint;
      }

      const osg::Vec3& getLocalIntersectNormal() const
      {
        return localIntersectionNormal;
      }

      osg::Vec3 getWorldIntersectNormal() const
      {
        using Matrix = osg::Matrix;

        return
          matrix.valid() ?
            Matrix::transform3x3(
              Matrix::inverse(*matrix),
              localIntersectionNormal
            ) :
            localIntersectionNormal;
      }
    };

    using Intersections = std::multiset<Intersection>;

    inline void insertIntersection(const Intersection& intersection)
    {
      getIntersections().insert(intersection);
    }

    inline Intersections& getIntersections()
    {
      return _parent ? _parent->_intersections : _intersections;
    }

    inline Intersection getFirstIntersection()
    {
      Intersections& intersections = getIntersections();

      return
        intersections.empty() ?
          Intersection() :
          *(intersections.begin());
    }

    inline void setStart(const osg::Vec3d& start) { _start = start; }
    inline const osg::Vec3d& getStart() const { return _start; }

    inline void setEnd(const osg::Vec3d& end) { _end = end; }
    inline const osg::Vec3d& getEnd() const { return _end; }

  public:

    virtual Intersector* clone(osgUtil::IntersectionVisitor& iv);
    virtual bool enter(const osg::Node& node);
    virtual void leave();

    virtual void
      intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable);

    virtual void reset();

    virtual bool
      containsIntersections() { return !getIntersections().empty(); }

  protected:

    // Internal constructor for used for clone request
    IntersectorPrivate();
    bool intersects(const osg::BoundingSphere& bs);

    bool
      intersectAndClip(
         osg::Vec3d& s,
         osg::Vec3d& e,
         const osg::BoundingBox& bb
      );

    IntersectorPrivate* _parent;

    osg::Vec3d  _start;
    osg::Vec3d  _end;
    osg::Vec3d  _thickness;

    Intersections _intersections;
};

