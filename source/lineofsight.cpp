#include "uox3.h"
#include "cmath"
#include "regions.h"

namespace UOX
{

#define MAX_COLLISIONS 1024

// Contains all current line of sight functionality

/*

  PROPOSED 4 pass Line of Sight algorithm that will take into account current issues.
  Note that it may actually be more suited to a client than server.

Line of Sight algorithm

4 pass algorithm
Pass 1:		Map
Pass 2:		Statics
Pass 3:		Dynamic Items
Pass 4:		Multis		// this pass is often ignored, as multis are rare

Failure at any pass returns false

Flags:

TREES and BUSHES
WALLS_CHIMNEYS
DOORS
ROOFING_SLANTED
FLOORS_FLAT_ROOFING
LAVA_WATER
NO_MAP

ONLY if all tests run through to their conclusion will you ever return a true value

General algorithm:

	// Different Xs
	Find dY
	Find dZ
	// Different Ys
	Find dX
	Find dZ
	// Same X/Y
	// Special case


	// Only pass 1 and 2 are different based on each of these cases
Different Xs algorithm
	
PASS 1
	if map check
		For each x between src and trg
			Increment Y by dY
			Increment Z by dZ
			Get map tile at x/y
			If no overlap z, discard
			elseif flag no match, discard
			else continue
		next x

PASS 2
	For each x between src and trg
		Increment Y by dY
		Increment Z by dZ
		Get static tile at x/y
		If no overlap z, discard
		elseif flag no match, discard
		else continue
	next x

Different Ys algorithm
PASS 1
	if map check
		for each y between src and trg
			Increment X by dX
			Increment Z by dZ
			Get map tile at x/y
			if no overlap z, discard
			elseif flag no match, discard
			else continue
		next y
PASS 2
	for each y between src and trg
		Increment X by dX
		Increment Z by dZ
		Get static tile at x/y
		if no overlap z, discard
		elseif flag no match, discard
		else continue
	next y
Different Zs algorithm
PASS 1
	if map check
		Find tile at x/y
		if z is not between min z and max z, discard
		if flag overlap, blocked
PASS 2
	for each static tile at x/y
		if z is not between min z and max z, discard
		if flag overlap, blocked	
	next tile
PASS 3
	For each item in the same map area
		if item.X < min X, discard
		if item.Y < min Y, discard
		if item.Z < min Z, discard
		if item.X > max X, discard
		if item.Y > min Y, discard
		if item.Z > min Z, discard
		Get item flags
		if no flag overlap, discard
		Construct 6 quads of the item
		Push quads onto list
	Next item
	For each quad in list
		Do intersect test of ray and quad
		if intersect, return false
	next quad

PASS 4
	For each item in the multi
		if item.X < min X, discard
		if item.Y < min Y, discard
		if item.Z < min Z, discard
		if item.X > max X, discard
		if item.Y > min Y, discard
		if item.Z > min Z, discard
		Get item flags
		if no flag overlap, discard
		Construct 6 quads of the item
		Push quads onto list
	Next item
	for each quad in list
		Do intersect test of ray and quad
		if intersect, return false
	next quad

QUAD POLYGON CONSTRUCTION ALGORITHM
	Need to create 6 polys
	One polygon per side of rectangular prism
	Need to use x/y offset
	Also need to use height of tile from tiledata.mul
*/

//o--------------------------------------------------------------------------
//|	Function		-	bool LineOfSight( cSocket *mSock, CChar *mChar, SI16 x2, SI16 y2, SI08 z2, int checkfor )
//|	Date			-	03 July, 2001
//|	Programmer		-	Abaddon
//|	Modified		-	18 March, 2002, sereg
//o--------------------------------------------------------------------------
//|	Purpose			-	Returns true if there is line of sight between src and trg
//o--------------------------------------------------------------------------
bool LineOfSight( cSocket *mSock, CChar *mChar, SI16 koxn, SI16 koym, SI08 koz2, int checkfor )
{
/*
Char (x1, y1, z1) is the char(pc/npc),  Target (x2, y2, z2) is the target.
s is for pc's, in case a message needs to be sent.
the checkfor is what is checked for along the line of sight.  
Look at uox3.h to see options. Works like npc magic.

  #define TREES_BUSHES 1 // Trees and other large vegetaion in the way
  #define WALLS_CHIMNEYS 2  // Walls, chimineys, ovens, etc... in the way
  #define DOORS 4 // Doors in the way
  #define ROOFING_SLANTED 8  // So can't tele onto slanted roofs, basically
  #define FLOORS_FLAT_ROOFING 16  //  For attacking between floors
  #define LAVA_WATER 32  // Don't know what all to use this for yet
  
	Just or (|) the values for the diff things together to get what to search for.
	So put in place of the paramater checkfor for example
	
	  if( line_of_sight( s, x1, y1, z1, x2, y2, z2, WALLS_CHIMNEYS | DOORS | ROOFING_SLANTED ) )
	  
		
		  This whole thing is based on the Pythagorean Theorem.  What I have done is
		  taken the coordinates from both chars and created a right triange with the
		  hypotenuse as the line of sight.  Then by sectioning off side "a" into a number
		  of equal lengths and finding the other sides lengths according to that one,  I 
		  have been able to find the coordinates of the tiles that lie along the line of
		  sight(line "c").  Then these tiles are searched from an item that would block 
		  the line of sight. 
		  
			<----- it WAS based on the P.T., now its based on linear algebra ;) (sereg)
	*/

	bool blocked = false;
	bool not_blocked = true;

	if( koxn == -1 && koym == -1 )  
		return not_blocked;		// target canceled

	SI16 kox1 = mChar->GetX(), koy1 = mChar->GetY();
	SI08 koz1 = mChar->GetZ();

	if( (kox1 == koxn) && (koy1 == koym) && (koz1 == koz2) )
		return not_blocked;		// if source and target are on the same position

	koz1 += 15; // standard eye height of most bodies

	UI08 worldNumber = mChar->WorldNumber();

	SI32 n = ( koxn - kox1 ), m = ( koym - koy1 ), i = 0;
	SI08 sgn_x = ( kox1 <= koxn ) ? 1 : (-1); // signum for x
	SI08 sgn_y = ( koy1 <= koym ) ? 1 : (-1); // signum for y
	SI08 sgn_z = ( koz1 <= koz2 ) ? 1 : (-1); // signum for z
	if( kox1 == koxn )
		sgn_x = 0;
	if( koy1 == koym )
		sgn_y = 0;
	if( koz1 == koz2 )
		sgn_z = 0;

	line3D lineofsight = line3D( vector3D( kox1, koy1, koz1 ), vector3D( (koxn-kox1), (koym-koy1), (koz2-koz1) ) );
	double rBlah = abs( koxn - kox1 ) * abs( koxn - kox1 ) + abs( koym - koy1 ) * abs( koym - koy1 );
	SI32 distance = (SI32)sqrt(rBlah);

	if( distance > 18 )
		return blocked;

	vector3D collisions[ MAX_COLLISIONS ];

	// initialize array
	for( i = 0 ; i < (distance * 2); ++i )
		collisions[i] = vector3D( -1, -1, -1 );

	SI32 collisioncount = 0;
	SI32 dz = 0; // dz is needed later for collisions of the ray with floor tiles
	if( sgn_x == 0 || m > n )
		dz = (SI32)floor( lineofsight.dzInDirectionY() );
	else
		dz = (SI32)floor( lineofsight.dzInDirectionX() );

	
	if( sgn_x == 0 && sgn_y == 0 && !sgn_z == 0 ) // should fix shooting through floor issues
	{
		for( i = 0; i < abs( koz2 - koz1 ); ++i )
		{
			collisions[collisioncount] = vector3D( kox1, koy1, koz1 + sgn_z );
			++collisioncount;
		}
	}
	else if( sgn_x == 0 ) // if we are on the same x-level, just push every x/y coordinate in y-direction from src to trg into the array
	{
		for( i = 0; i < (sgn_y * m); ++i )
		{
			collisions[collisioncount] = vector3D( kox1, koy1 + (sgn_y * i), (signed char)(koz1 + floor(lineofsight.dzInDirectionY() * (R32)i)) );
			++collisioncount;
		}
	}
	else if( sgn_y == 0 ) // if we are on the same y-level, just push every x/y coordinate in x-direction from src to trg into the array
	{
		for( i = 0; i < (sgn_x * n); ++i )
		{
			collisions[collisioncount] = vector3D( kox1 + (sgn_x * i), koy1, (signed char)(koz1 + floor(lineofsight.dzInDirectionX() * (R32)i)) );
			++collisioncount;
		}
	}
	else
	{
		for( i = 0; (n >= m) && (i < (sgn_x * n)); ++i )
		{
			line2D toCollide = line2D( vector2D( (R32)( kox1 + (sgn_x * i) ), 0.0f ), vector2D( 0.0f, 1.0f ) );
			vector2D temp = lineofsight.Projection2D().CollideLines2D( toCollide );

			if( ( temp.x != -1 ) && ( temp.y != -1 ) )
			{
				// the next one is somewhat tricky, if the line of sight exactly cuts a coordinate,
				// we just have to take that coordinate...
				if( floor( temp.y ) == temp.y ) 
				{
					collisions[collisioncount] = ( vector3D( (long)floor( temp.x ), (long)floor( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionX() * (R32)i)) ) ); 
					collisioncount += 1;
				}
				// but if not, we have to take BOTH coordinates, which the calculated collision is between!
				else
				{ 
					collisions[collisioncount] = ( vector3D( (long)floor( temp.x ), (long)floor( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionX() * (R32)i)) ) ); 
					collisions[collisioncount+1] = ( vector3D( (long)ceil( temp.x ), (long)ceil( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionX() * (R32)i)) ) ); 
					collisioncount += 2;
				}
			}
		}
	
		for( i = 0; (m > n) && (i < (sgn_y * m)); ++i )
		{
			line2D toCollide = line2D( vector2D( 0.0f, (R32)( koy1 + (sgn_y * i) ) ), vector2D( 1.0f, 0.0f ) );
			vector2D temp = lineofsight.Projection2D().CollideLines2D( toCollide );

			if( ( temp.x != -1 ) && ( temp.y != -1 ) )
			{
				if( floor( temp.x ) == temp.x )
				{
					collisions[collisioncount] = ( vector3D( (long)floor( temp.x ), (long)floor( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionY() * (R32)i)) ) ); 
					collisioncount += 1;
				}
				else
				{
					collisions[collisioncount] = ( vector3D( (long)floor( temp.x ), (long)floor( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionY() * (R32)i)) ) ); 
					collisions[collisioncount+1] = ( vector3D( (long)ceil( temp.x ), (long)ceil( temp.y ), (signed char)(koz1 + floor(lineofsight.dzInDirectionY() * (R32)i)) ) ); 
					collisioncount += 2;
				}
			}
		}
	}
	///////////////////////////////////////////////////////////
	/////////////////  These next lines initialize arrays
	/*
	This function has to search the items array a number of times which
	caused a bit of lag.  I made this item cache to be used instead.  
	The items array is only search once for items in a 40 tile area.
	if an item is found, it is put into the cache, which is then used
	by the rest of the function.  This way it doesn't have to check the 
	entire array each time.
	*/
	// - Tauriel's region stuff 3/6/99

	ITEMLIST loscache;
	std::vector< UI16 > itemids;
	SI16 x1, y1, x2, y2;
	if( kox1 < koxn )
	{
		x1 = kox1;
		x2 = koxn;
	}
	else
	{
		x1 = koxn;
		x2 = kox1;
	}
	if( koy1 < koym )
	{
		y1 = koy1;
		y2 = koym;
	}
	else
	{
		y1 = koym;
		y2 = koy1;
	}

	REGIONLIST nearbyRegions = MapRegion->PopulateList( kox1, koy1, worldNumber );
	REGIONLIST_ITERATOR rIter;
	for( rIter = nearbyRegions.begin(); rIter != nearbyRegions.end(); ++rIter )
	{
		SubRegion *MapArea = (*rIter);
		if( MapArea == NULL )	// no valid region
			continue;
		MapArea->itemData.Push();
		for( CItem *tempItem = MapArea->itemData.First(); !MapArea->itemData.Finished(); tempItem = MapArea->itemData.Next() )
		{
			if( !ValidateObject( tempItem ) )
				continue;
			if( ( tempItem->GetX() >= x1 ) &&
				( tempItem->GetX() <= x2 ) &&
				( tempItem->GetY() >= y1 ) &&
				( tempItem->GetY() <= y2 ) )
				for( i = 0; i < collisioncount; ++i )
				{
					if( ( tempItem->GetX() == collisions[i].x ) && 
						( tempItem->GetY() == collisions[i].y ) )
					{
						loscache.push_back( tempItem );
					}
				}
		}
		MapArea->itemData.Pop();
	}
		
		
	////////////End Initilzations
	//////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////
	////////////////  This determines what to check for
	SI32 checkthis[ITEM_TYPE_CHOICES];
	SI32 checkthistotal = 0;
	SI32 itemtype = 1;

	i = 0;

	while( checkfor )
	{
		if( ( checkfor >= itemtype ) && ( checkfor < ( itemtype * 2 ) ) && ( checkfor ) )
		{
			checkthis[i] = itemtype;
			++i;
			checkfor = (checkfor - itemtype);
			++checkthistotal;
			itemtype = 1;
		}
		else if( checkfor )
			itemtype *= 2;
	}    


	///////////////////////////////////////////////////////////////////////////
	////////////////////  This next stuff is what searches each tile for things
	map_st map1, map2;
	SI32 length, j;

	for( i = 0; i < collisioncount; ++i )
	{
		MapStaticIterator msi( collisions[i].x, collisions[i].y, worldNumber );
		// Texture mapping  
		map1 = Map->SeekMap0( static_cast<SI16>(collisions[i].x), static_cast<SI16>(collisions[i].y), worldNumber );
		map2 = Map->SeekMap0( static_cast<SI16>(collisions[i].x + sgn_x), static_cast<SI16>(collisions[i].y + sgn_y), worldNumber );
		
		if( (map1.id != 2) && (map2.id != 2) ) 
		{
			// Mountain walls
			if( ( ( map1.z < map2.z ) &&						// 1) lineofsight collides with a map "wall"
				( collisions[i].z <= ( map2.z ) ) &&
				( collisions[i].z >= ( map1.z ) ) ) ||
				( ( map1.z > map2.z ) &&
				( collisions[i].z <= ( map1.z ) ) &&
				( collisions[i].z >= ( map2.z ) ) ) ||
				( ( collisions[i].z == map1.z ) &&				// 2) lineofsight cuts a map "floor"
				( lineofsight.dir.z != 0 ) ) ||
				( ( ( map1.id >= 431  && map1.id <= 432  ) ||	// 3) lineofsight cuts a mountain
				( map1.id >= 467  && map1.id <= 475  ) ||
				( map1.id >= 543  && map1.id <= 560  ) ||
				( map1.id >= 1754 && map1.id <= 1757 ) ||
				( map1.id >= 1787 && map1.id <= 1789 ) ||
				( map1.id >= 1821 && map1.id <= 1824 ) ||
				( map1.id >= 1851 && map1.id <= 1854 ) ||
				( map1.id >= 1881 && map1.id <= 1884 ) ) &&
				( msi.First() == NULL ) ) ) // make sure there is no static item!
			{
				if( mSock != NULL )
					mSock->sysmessage( 683 );
				return blocked;
			}
		}	 
			
		// Statics
		CTile tile;
		staticrecord *stat = msi.First();
		while( stat != NULL )
		{
			msi.GetTile( &tile );
			if(	( collisions[i].z >= stat->zoff && collisions[i].z <= ( stat->zoff + tile.Height() ) ) ||
				( tile.Height() <= 2 && abs( collisions[i].z - stat->zoff ) <= abs( dz ) ) )
			{	  
				itemids.push_back( stat->itemid );
			}
			stat = msi.Next();
		}
			
			
		// Items
		CItem *dyncount;
		ITEMLIST_ITERATOR losIter;
		for( losIter = loscache.begin(); losIter != loscache.end(); ++losIter ) 
		{
			dyncount = (*losIter);
			if( dyncount == NULL )
				continue;
			if( dyncount->GetID( 1 ) < 0x40 )
			{ // Dynamic items
				Map->SeekTile( dyncount->GetID(), &tile);
				if( ( dyncount->GetX() == collisions[i].x ) && (dyncount->GetY() == collisions[i].y ) &&
					( collisions[i].z >= dyncount->GetZ() ) && ( collisions[i].z <= ( dyncount->GetZ() + tile.Height() ) ) &&
					( dyncount->GetVisible() == VT_VISIBLE ) )
				{	
					itemids.push_back( dyncount->GetID() );
				}
			}
			else
			{// Multi's
				if( ( abs( kox1 - koxn ) <= DIST_BUILDRANGE ) && ( abs( koy1 - koym ) <= DIST_BUILDRANGE ) )
				{
					st_multi *test;
					int multiID = ( dyncount->GetID() ) - 0x4000;
					Map->SeekMulti( multiID, &length );
					if( length == -1 || length >= 17000000 )//Too big... bug fix hopefully (Abaddon 13 Sept 1999)
					{
						Console << "LoS - Bad length in multi file. Avoiding stall" << myendl;
						length = 0;
					}
					for( j = 0; j < length; ++j )
					{
						test = Map->SeekIntoMulti( multiID, j );
						if( ( test->visible ) && ( dyncount->GetX() + test->x == collisions[i].x ) &&
							( dyncount->GetY() + test->y == collisions[i].y ) )			
						{
							Map->SeekTile(test->tile, &tile);
							if( ( collisions[i].z >= dyncount->GetZ() + test->z ) &&
								( collisions[i].z <= (dyncount->GetZ() + test->z + tile.Height() ) ) )
							{
								itemids.push_back( test->tile );
							}
						}
					}
				}
			}
		}
	} // for loop
 
	for( size_t toCheck = 0; toCheck < itemids.size(); ++toCheck )
	{
		for( j = 0; j < checkthistotal; ++j )
		{
			switch( checkthis[j] )
			{
				case 1 : // Trees, Shrubs, bushes
					if( ( itemids[toCheck] == 3240 ) || ( itemids[toCheck] == 3242 ) || ( ( itemids[toCheck] >= 3215 ) && ( itemids[toCheck] <= 3218 ) ) ||
					( ( itemids[toCheck] >= 3272 ) && ( itemids[toCheck] <= 3280 ) ) || ( itemids[toCheck] == 3283 ) || ( itemids[toCheck] == 3286 ) ||
					( itemids[toCheck] == 3288 ) || ( itemids[toCheck] == 3290 ) || ( itemids[toCheck] == 3293 ) || ( itemids[toCheck] == 3296 ) ||
					( itemids[toCheck] == 3299 ) || ( itemids[toCheck] == 3302 ) || ( itemids[toCheck] == 3305 ) || ( itemids[toCheck] == 3306 ) ||
					( itemids[toCheck] == 3320 ) || ( itemids[toCheck] == 3323 ) || ( itemids[toCheck] == 3326 ) || ( itemids[toCheck] == 3329 ) ||
					( itemids[toCheck] == 3381 ) || ( itemids[toCheck] == 3383 ) || ( itemids[toCheck] == 3384 ) || ( itemids[toCheck] == 3394 ) ||
					( itemids[toCheck] == 3395 ) || ( ( itemids[toCheck] >= 3416 ) && ( itemids[toCheck] <= 3418 ) ) ||
					( itemids[toCheck] == 3440 ) || ( itemids[toCheck] == 3461 ) || ( itemids[toCheck] == 3476 ) || ( itemids[toCheck] == 3480 ) ||
					( itemids[toCheck] == 3484 ) || ( itemids[toCheck] == 3488 ) || ( itemids[toCheck] == 3492 ) || ( itemids[toCheck] == 3496 ) ||
					( itemids[toCheck] == 3512 ) || ( itemids[toCheck] == 3513 ) || ( ( itemids[toCheck] >= 4792 ) && ( itemids[toCheck] <= 4795 ) ) )
					{
						return blocked;
					}
					break;
				case 2 : // Walls, Chimneys, ovens, not fences
					if( ( ( itemids[toCheck] >= 6 ) && ( itemids[toCheck] <= 748 ) ) || ( ( itemids[toCheck] >= 761 ) && ( itemids[toCheck] <= 881 ) ) ||
					( ( itemids[toCheck] >= 895 ) && ( itemids[toCheck] <= 1006 ) ) || ( ( itemids[toCheck] >= 1057 ) && ( itemids[toCheck] <= 1061 ) ) ||
					( itemids[toCheck] == 1072 ) || ( itemids[toCheck] == 1073 ) || ( ( itemids[toCheck] >= 1080 ) && ( itemids[toCheck] <= 1166 ) ) ||
					( ( itemids[toCheck] >= 2347 ) && ( itemids[toCheck] <= 2412 ) ) || ( ( itemids[toCheck] >= 16114 ) && ( itemids[toCheck] <= 16134 ) ) ||
					( ( itemids[toCheck] >= 8538 ) && ( itemids[toCheck] <= 8553 ) ) || ( ( itemids[toCheck] >= 9535 ) && ( itemids[toCheck] <= 9555 ) ) ||
					( itemids[toCheck] == 12583 ) ||
					( ( itemids[toCheck] >= 1801 ) && ( itemids[toCheck] <= 2000 ) ) ) //stairs
					{
						return blocked;
					}
					break;
				case 4 : // Doors, not gates
					if( ( ( itemids[toCheck] >= 1653 ) && ( itemids[toCheck] <= 1782 ) ) || ( ( itemids[toCheck] >= 8173 ) && ( itemids[toCheck] <= 8188 ) ) )
					{
						return blocked;
					}
				break;
				case 8 : // Roofing Slanted
					if( ( ( itemids[toCheck] >= 1414 ) && ( itemids[toCheck] <= 1578 ) ) || ( ( itemids[toCheck] >= 1587 ) && ( itemids[toCheck] <= 1590 ) ) ||
					( ( itemids[toCheck] >= 1608 ) && ( itemids[toCheck] <= 1617 ) ) || ( ( itemids[toCheck] >= 1630 ) && ( itemids[toCheck] <= 1652 ) ) ||
					( ( itemids[toCheck] >= 1789 ) && ( itemids[toCheck] <= 1792 ) ) )
					{
						return blocked;
					}
				break;
				case 16 : // Floors & Flat Roofing (Attacking through floors Roofs)
					if( ( ( itemids[toCheck] >= 1169 ) && ( itemids[toCheck] <= 1413 ) ) || ( ( itemids[toCheck] >= 1508 ) && ( itemids[toCheck] <= 1514 ) ) ||
					( ( itemids[toCheck] >= 1579 ) && ( itemids[toCheck] <= 1586 ) ) || ( ( itemids[toCheck] >= 1591 ) && ( itemids[toCheck] <= 1598 ) ) )
					{
						if( (koz1-15) == koz2 ) // in case of char and target on same roof
							return not_blocked;
						else
							return blocked;
					}
					break;
				case 32 :  // Lava, water
					if( ( ( itemids[toCheck] >= 4846 ) && ( itemids[toCheck] <= 4941 ) ) || ( ( itemids[toCheck] >= 6038 ) && ( itemids[toCheck] <= 6066 ) ) ||
					( ( itemids[toCheck] >= 12934 ) && ( itemids[toCheck] <= 12977 ) ) || ( ( itemids[toCheck] >= 13371 ) && ( itemids[toCheck] <= 13420 ) ) ||
					( ( itemids[toCheck] >= 13422 ) && ( itemids[toCheck] <= 13638 ) ) || ( ( itemids[toCheck] >= 13639 ) && ( itemids[toCheck] <= 13665 ) ) )
					{
						return blocked;
					}
					break;
				default:
					return not_blocked;
			}
		}
	}
	return not_blocked;
}

}
